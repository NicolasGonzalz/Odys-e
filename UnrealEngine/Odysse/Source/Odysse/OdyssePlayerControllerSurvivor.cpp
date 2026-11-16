// Choix du survivant : le premier écran en entrant dans une partie (docs/HISTOIRE.md § 4).
// Hors ligne, on entre aussitôt ; en ligne, le serveur confirme (ou répond qu'il vient d'être pris).

#include "OdyssePlayerController.h"

#include "Components/Button.h"
#include "UI/OdysseeMainMenuWidget.h"
#include "UI/OdysseeSurvivorChoiceWidget.h"

#define LOCTEXT_NAMESPACE "OdysseController"

void AOdyssePlayerController::CreateSurvivorChoice()
{
	SurvivorChoice = CreateWidget<UOdysseeSurvivorChoiceWidget>(this);
	SurvivorChoice->OnSurvivorChosen.BindUObject(this, &AOdyssePlayerController::OnSurvivorPicked);
	SurvivorChoice->BackButton->OnClicked.AddDynamic(this, &AOdyssePlayerController::OnSurvivorChoiceBack);
	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->OnSurvivorsChanged.AddUniqueDynamic(this, &AOdyssePlayerController::OnSurvivorsChanged);
	}
}

void AOdyssePlayerController::ShowSurvivorChoice(const FText& Status)
{
	MainMenu->RemoveFromParent();
	RefreshSurvivorChoice();
	SurvivorChoice->ShowStatus(Status);
	if (!SurvivorChoice->IsInViewport())
	{
		SurvivorChoice->AddToViewport(10);
	}
	SetMenuInput(SurvivorChoice);
}

void AOdyssePlayerController::RefreshSurvivorChoice()
{
	TArray<FOdysseeSurvivorView> Views;
	if (const UOdysseeNetSubsystem* Net = GetNet())
	{
		for (const FOdysseeSurvivor& Survivor : Net->GetSurvivors())
		{
			Views.Add({Survivor.Index, Survivor.Name, Survivor.Capsule, Survivor.Specialty, Survivor.Skill,
				Survivor.bTaken});
		}
	}
	SurvivorChoice->ShowSurvivors(Views);
}

void AOdyssePlayerController::OnSurvivorPicked(int32 Index)
{
	UOdysseeNetSubsystem* Net = GetNet();
	if (!Net || !Net->IsConnected())
	{
		ShowGame(); // hors ligne : personne d'autre, on entre aussitôt
		return;
	}
	PendingSurvivor = Index;
	Net->RequestChooseSurvivor(Index);
	SurvivorChoice->ShowStatus(LOCTEXT("Waking", "Ouverture de la capsule…"));
}

void AOdyssePlayerController::OnSurvivorsChanged()
{
	const UOdysseeNetSubsystem* Net = GetNet();
	if (!Net || !SurvivorChoice->IsInViewport())
	{
		return;
	}
	if (Net->GetMySurvivor() >= 0)
	{
		PendingSurvivor = -1;
		ShowGame(); // le serveur a confirmé : on se réveille
		return;
	}
	RefreshSurvivorChoice(); // quelqu'un vient d'en prendre un : sa carte se grise
	if (PendingSurvivor >= 0)
	{
		PendingSurvivor = -1;
		SurvivorChoice->ShowStatus(LOCTEXT("JustTaken", "Ce survivant vient d'être pris : choisissez-en un autre."));
	}
}

void AOdyssePlayerController::OnSurvivorChoiceBack()
{
	if (UOdysseeNetSubsystem* Net = GetNet(); Net && Net->IsConnected())
	{
		Net->Disconnect();
	}
	ShowMenu(FText::GetEmpty());
}

#undef LOCTEXT_NAMESPACE
