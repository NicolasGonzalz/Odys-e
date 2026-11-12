// Aperçu de l'interface, en attendant qu'elle soit branchée sur le jeu : pendant une partie (Play),
// ouvrir la console (touche ² ou `) et taper
//     Odyssee.Ui hud | menu | pause | coop | aucun
// L'écran choisi s'affiche avec des valeurs de démonstration ; « aucun » le retire.

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UI/OdysseeHudWidget.h"
#include "UI/OdysseeMainMenuWidget.h"
#include "UI/OdysseePauseMenuWidget.h"

namespace
{
	TWeakObjectPtr<UUserWidget> Shown;

	UUserWidget* Create(const FString& What, APlayerController* Player)
	{
		if (What == TEXT("hud"))
		{
			UOdysseeHudWidget* Hud = CreateWidget<UOdysseeHudWidget>(Player);
			Hud->ShowPreviewValues();
			return Hud;
		}
		if (What == TEXT("menu"))
		{
			return CreateWidget<UOdysseeMainMenuWidget>(Player);
		}
		if (What == TEXT("pause") || What == TEXT("coop"))
		{
			UOdysseePauseMenuWidget* Pause = CreateWidget<UOdysseePauseMenuWidget>(Player);
			Pause->SetCoopMode(What == TEXT("coop"));
			return Pause;
		}
		return nullptr;
	}

	/** Souris visible sur les menus ; le jeu garde le clavier (et la console reste accessible). */
	void SetInput(APlayerController* Player, UUserWidget* Widget, bool bMenu)
	{
		if (bMenu)
		{
			FInputModeGameAndUI Mode;
			Mode.SetWidgetToFocus(Widget->TakeWidget());
			Player->SetInputMode(Mode);
		}
		else
		{
			Player->SetInputMode(FInputModeGameOnly());
		}
		Player->SetShowMouseCursor(bMenu);
	}

	void ShowPreview(const TArray<FString>& Args, UWorld* World)
	{
		if (Shown.IsValid())
		{
			Shown->RemoveFromParent();
			Shown.Reset();
		}
		APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		if (Player == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Odyssee.Ui : lancer d'abord une partie (Play)."));
			return;
		}
		const FString What = Args.Num() > 0 ? Args[0].ToLower() : TEXT("hud");
		UUserWidget* Widget = Create(What, Player);
		if (Widget == nullptr)
		{
			SetInput(Player, nullptr, false);
			return; // « aucun », ou un nom inconnu : rien à afficher
		}
		Widget->AddToViewport(10);
		Shown = Widget;
		SetInput(Player, Widget, What != TEXT("hud"));
	}

	FAutoConsoleCommandWithWorldAndArgs PreviewCommand(TEXT("Odyssee.Ui"),
		TEXT("Aperçu de l'interface : hud, menu, pause, coop (pause en coop), aucun."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowPreview));
}
