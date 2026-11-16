// Aperçu de l'interface, en attendant qu'elle soit branchée sur le jeu : pendant une partie (Play),
// ouvrir la console (touche ² ou `) et taper
//     Odyssee.Ui hud | menu | pause | coop | inventaire | artisanat | aucun
// L'écran choisi s'affiche avec des valeurs de démonstration ; « aucun » le retire.

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UI/OdysseeCraftingWidget.h"
#include "UI/OdysseeHudWidget.h"
#include "UI/OdysseeInventoryWidget.h"
#include "UI/OdysseeMainMenuWidget.h"
#include "UI/OdysseePauseMenuWidget.h"

namespace
{
	TWeakObjectPtr<UUserWidget> Shown;

	UUserWidget* CreateInventory(APlayerController* Player)
	{
		UOdysseeInventoryWidget* Inventory = CreateWidget<UOdysseeInventoryWidget>(Player);
		TArray<FOdysseeSlotView> Views;
		Views.SetNum(UOdysseeInventoryWidget::SlotCount);
		Views[0] = {TEXT("outil_hache_pierre"), 1, FText::FromString(TEXT("Hache de pierre"))};
		Views[1] = {TEXT("outil_pioche_pierre"), 1, FText::FromString(TEXT("Pioche de pierre"))};
		Views[3] = {TEXT("outil_lampe"), 1, FText::FromString(TEXT("Lampe torche"))};
		Views[5] = {TEXT("outil_baton"), 12, FText::FromString(TEXT("Bâton"))};
		Views[6] = {TEXT("outil_lance"), 1, FText::FromString(TEXT("Lance"))};
		Inventory->ShowSlots(Views);
		return Inventory;
	}

	FOdysseeItemView Item(const TCHAR* Key, const TCHAR* Icon, const TCHAR* Name, int32 Count)
	{
		return {Key, Icon, FText::FromString(Name), Count};
	}

	FOdysseeRecipeView Recipe(const TCHAR* Key, const TCHAR* Category, const TCHAR* Label, const TCHAR* Station,
		float Seconds, const FOdysseeItemView& Output, const TArray<FOdysseeItemView>& Inputs)
	{
		return {Key, Category, FText::FromString(Label), FText::FromString(Station), Seconds, Output, Inputs};
	}

	UUserWidget* CreateCrafting(APlayerController* Player)
	{
		UOdysseeCraftingWidget* Crafting = CreateWidget<UOdysseeCraftingWidget>(Player);
		const FOdysseeItemView Stick = Item(TEXT("stick"), TEXT("objet_baton"), TEXT("Bâton"), 1);
		const FOdysseeItemView Stone = Item(TEXT("stone"), TEXT("objet_pierre"), TEXT("Pierre"), 2);
		const FOdysseeItemView Log = Item(TEXT("wood_log"), TEXT("objet_buche"), TEXT("Bûche"), 8);
		Crafting->ShowRecipes({
			Recipe(TEXT("stone_axe"), TEXT("tool"), TEXT("Outils"), TEXT("À la main"), 3.f,
				Item(TEXT("stone_axe"), TEXT("objet_hache_pierre"), TEXT("Hache de pierre"), 1), {Stick, Stone}),
			Recipe(TEXT("stone_pickaxe"), TEXT("tool"), TEXT("Outils"), TEXT("À la main"), 3.f,
				Item(TEXT("stone_pickaxe"), TEXT("objet_pioche_pierre"), TEXT("Pioche de pierre"), 1), {Stick, Stone}),
			Recipe(TEXT("torch"), TEXT("tool"), TEXT("Outils"), TEXT("À la main"), 2.f,
				Item(TEXT("torch"), TEXT("objet_torche"), TEXT("Torche"), 1), {Stick, Item(TEXT("cloth"), TEXT("objet_tissu"), TEXT("Tissu"), 1)}),
			Recipe(TEXT("planks"), TEXT("resource"), TEXT("Matériaux"), TEXT("À l'établi"), 2.f,
				Item(TEXT("planks"), TEXT("objet_planches"), TEXT("Planches"), 4), {Item(TEXT("wood_log"), TEXT("objet_buche"), TEXT("Bûche"), 1)}),
			Recipe(TEXT("workbench"), TEXT("station"), TEXT("Stations"), TEXT("À la main"), 6.f,
				Item(TEXT("workbench"), TEXT("objet_etabli"), TEXT("Établi"), 1), {Log, Item(TEXT("stone"), TEXT("objet_pierre"), TEXT("Pierre"), 6)}),
		});
		Crafting->ShowOwned({{TEXT("wood_log"), 3}, {TEXT("stone"), 6}, {TEXT("stick"), 4}});
		return Crafting;
	}

	UUserWidget* Create(const FString& What, APlayerController* Player)
	{
		if (What == TEXT("hud"))
		{
			UOdysseeHudWidget* Hud = CreateWidget<UOdysseeHudWidget>(Player);
			Hud->ShowPreviewValues();
			return Hud;
		}
		if (What == TEXT("inventaire"))
		{
			return CreateInventory(Player);
		}
		if (What == TEXT("artisanat"))
		{
			return CreateCrafting(Player);
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
		TEXT("Aperçu de l'interface : hud, menu, pause, coop (pause en coop), inventaire, artisanat, aucun."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowPreview));
}
