// Copyright Epic Games, Inc. All Rights Reserved.


#include "OdyssePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "OdysseCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Odysse.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OdysseCharacter.h"
#include "OdysseeMovementComponent.h"
#include "UI/OdysseeGaugeWidget.h"
#include "UI/OdysseeHudWidget.h"
#include "UI/OdysseeToolSlotWidget.h"
#include "UI/OdysseeUiStyle.h"
#include "UI/OdysseeMainMenuWidget.h"
#include "UI/OdysseePauseMenuWidget.h"

#define LOCTEXT_NAMESPACE "OdysseController"

AOdyssePlayerController::AOdyssePlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AOdysseCameraManager::StaticClass();
}

void AOdyssePlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	if (IsLocalPlayerController())
	{
		CreateScreens();
		UOdysseeNetSubsystem* Net = GetNet();
		if (Net && Net->IsConnected())
		{
			ShowGame(); // déjà en ligne (changement de carte)
		}
		else
		{
			ShowMenu(FText::GetEmpty());
		}
	}

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogOdysse, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AOdyssePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Échap : menu pause (hors des actions d'entrée du modèle, pour ne dépendre d'aucun asset)
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AOdyssePlayerController::TogglePause);

		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

// --- Écrans (T027) ------------------------------------------------------------------------------

void AOdyssePlayerController::CreateScreens()
{
	Hud = CreateWidget<UOdysseeHudWidget>(this);
	MainMenu = CreateWidget<UOdysseeMainMenuWidget>(this);
	PauseMenu = CreateWidget<UOdysseePauseMenuWidget>(this);
	PauseMenu->SetCoopMode(true); // en ligne, le serveur sauvegarde : pas de bouton Enregistrer

	MainMenu->JoinButton->OnClicked.AddDynamic(this, &AOdyssePlayerController::OnJoinClicked);
	MainMenu->NewWorldButton->OnClicked.AddDynamic(this, &AOdyssePlayerController::OnPlayOfflineClicked);
	MainMenu->QuitButton->OnClicked.AddDynamic(this, &AOdyssePlayerController::OnQuitGameClicked);
	PauseMenu->ResumeButton->OnClicked.AddDynamic(this, &AOdyssePlayerController::OnResumeClicked);
	PauseMenu->QuitButton->OnClicked.AddDynamic(this, &AOdyssePlayerController::OnLeaveClicked);

	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->OnConnectionChanged.AddUniqueDynamic(this, &AOdyssePlayerController::OnConnectionChanged);
	}
}

void AOdyssePlayerController::ShowMenu(const FText& Status)
{
	bInGame = false;
	Hud->RemoveFromParent();
	PauseMenu->RemoveFromParent();
	MainMenu->JoinStatusText->SetText(Status);
	if (!MainMenu->IsInViewport())
	{
		MainMenu->AddToViewport(10);
	}
	SetMenuInput(MainMenu);
}

void AOdyssePlayerController::ShowGame()
{
	bInGame = true;
	MainMenu->RemoveFromParent();
	PauseMenu->RemoveFromParent();
	if (!Hud->IsInViewport())
	{
		Hud->AddToViewport(0);
	}
	SetGameInput();
}

void AOdyssePlayerController::SetMenuInput(UUserWidget* Focus)
{
	FInputModeGameAndUI Mode; // le clavier reste au jeu : Échap ferme le menu pause
	Mode.SetWidgetToFocus(Focus->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
	SetIgnoreMoveInput(true); // le personnage ne bouge plus (voir AOdysseCharacter::DoMove)
	SetIgnoreLookInput(true);
}

void AOdyssePlayerController::SetGameInput()
{
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
}

void AOdyssePlayerController::TogglePause()
{
	if (!bInGame)
	{
		return;
	}
	if (PauseMenu->IsInViewport())
	{
		OnResumeClicked();
		return;
	}
	PauseMenu->AddToViewport(10);
	SetMenuInput(PauseMenu);
}

void AOdyssePlayerController::OnJoinClicked()
{
	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->Connect(MainMenu->AddressInput->GetText().ToString(), MainMenu->NameInput->GetText().ToString(),
			MainMenu->JoinCodeInput->GetText().ToString());
		MainMenu->JoinStatusText->SetText(Net->GetStatusText());
	}
}

void AOdyssePlayerController::OnPlayOfflineClicked()
{
	ShowGame(); // hors ligne : le personnage simule seul, sur la carte locale
}

void AOdyssePlayerController::OnQuitGameClicked()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AOdyssePlayerController::OnResumeClicked()
{
	PauseMenu->RemoveFromParent();
	SetGameInput();
}

void AOdyssePlayerController::OnLeaveClicked()
{
	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->Disconnect();
	}
	ShowMenu(LOCTEXT("Left", "Partie quittée"));
}

void AOdyssePlayerController::OnConnectionChanged(EOdysseeConnection State, const FText& Status)
{
	if (State == EOdysseeConnection::Connected)
	{
		ShowGame();
	}
	else if (State == EOdysseeConnection::Failed)
	{
		ShowMenu(Status); // refus, serveur muet ou connexion perdue : on dit pourquoi
		MainMenu->ShowMultiplayer();
	}
	else
	{
		MainMenu->JoinStatusText->SetText(Status);
	}
}

// --- HUD ----------------------------------------------------------------------------------------

void AOdyssePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (bInGame && Hud)
	{
		UpdateHud();
	}
}

void AOdyssePlayerController::UpdateHud()
{
	if (const AOdysseCharacter* Survivor = Cast<AOdysseCharacter>(GetPawn()))
	{
		Hud->StaminaBar->SetPercent(Survivor->GetSimMovement()->GetStamina());
		// Case tenue (touches 1 à 5, T047) : encadrée dans la barre d'outils.
		const int32 Held = Survivor->GetSimMovement()->GetHeldSlot();
		if (Held != ShownHeldSlot) // seulement au changement : SetActive recharge une texture
		{
			ShownHeldSlot = Held;
			for (int32 Index = 0; Index < Hud->ToolSlots.Num(); ++Index)
			{
				Hud->ToolSlots[Index]->SetActive(Index == Held);
			}
		}
	}

	const UOdysseeNetSubsystem* Net = GetNet();
	const bool bOnline = Net && Net->IsConnected();
	UpdateToolbar(bOnline ? Net->GetInventory() : TArray<FOdysseeInventorySlot>());
	// Besoins vitaux (T051) : calculés par le serveur ; pleins hors ligne.
	Hud->HealthBar->SetPercent(bOnline ? Net->GetHealth() : 1.0f);
	Hud->HungerGauge->SetProgress(bOnline ? Net->GetHunger() : 1.0f);
	Hud->ThirstGauge->SetProgress(bOnline ? Net->GetThirst() : 1.0f);

	if (!bOnline)
	{
		Hud->PingText->SetText(FText::GetEmpty());
		Hud->ConnectionText->SetText(LOCTEXT("Offline", "Hors ligne"));
		return;
	}
	const int32 Ping = Net->GetPingMs();
	Hud->PingText->SetText(Ping < 0 ? FText::GetEmpty()
		: FText::Format(LOCTEXT("Ping", "{0} ms"), FText::AsNumber(Ping)));
	const float Loss = Net->GetLossPercent();
	Hud->ConnectionText->SetText(Loss >= 1.0f
		? FText::Format(LOCTEXT("Loss", "Connecté · pertes {0} %"), FText::AsNumber(FMath::RoundToInt(Loss)))
		: LOCTEXT("Connected", "Connecté"));
}

void AOdyssePlayerController::UpdateToolbar(const TArray<FOdysseeInventorySlot>& Inventory)
{
	for (int32 Index = 0; Index < Hud->ToolSlots.Num(); ++Index)
	{
		const FOdysseeInventorySlot* Cell = Inventory.IsValidIndex(Index) ? &Inventory[Index] : nullptr;
		const FString Key = Cell && Cell->Count > 0
			? FString::Printf(TEXT("%s#%d"), *Cell->IconName, Cell->Count) : FString();
		if (ShownToolbar.IsValidIndex(Index) && ShownToolbar[Index] == Key)
		{
			continue; // inchangée : rien à redessiner
		}
		ShownToolbar.SetNum(Hud->ToolSlots.Num());
		ShownToolbar[Index] = Key;
		const FString Icon = Key.IsEmpty() ? FString() : IconFor(Cell->IconName);
		const int32 Count = Key.IsEmpty() ? 0 : Cell->Count;
		Hud->ToolSlots[Index]->ShowItem(Icon.IsEmpty() ? nullptr : *Icon, Count, -1.f);
	}
}

FString AOdyssePlayerController::IconFor(const FString& BaseName)
{
	// Icône : T_Ui_objet_<nom> (art/exports/icones/<nom>.png importée), sinon T_Ui_outil_<nom>,
	// sinon aucune (la case montre seulement la quantité). Cherchée une fois par objet.
	if (const FString* Known = IconCache.Find(BaseName))
	{
		return *Known;
	}
	FString Found;
	for (const TCHAR* Prefix : {TEXT("objet_"), TEXT("outil_")})
	{
		if (Found.IsEmpty() && !BaseName.IsEmpty() && OdysseeUi::Texture(*(Prefix + BaseName)))
		{
			Found = Prefix + BaseName;
		}
	}
	return IconCache.Add(BaseName, Found);
}

UOdysseeNetSubsystem* AOdyssePlayerController::GetNet() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UOdysseeNetSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
