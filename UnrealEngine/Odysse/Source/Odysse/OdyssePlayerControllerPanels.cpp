// Panneaux du joueur : inventaire (Tab) et artisanat (C). Ils s'ouvrent par-dessus le jeu sans
// jamais le mettre en pause : on peut continuer à marcher, la souris sert au panneau.

#include "OdyssePlayerController.h"

#include "UI/OdysseeCraftingWidget.h"
#include "UI/OdysseeHudWidget.h"
#include "UI/OdysseeInventoryWidget.h"
#include "UI/OdysseePauseMenuWidget.h"

#define LOCTEXT_NAMESPACE "OdysseController"

namespace
{
	FText StationName(FName Station)
	{
		if (Station == TEXT("workbench"))
		{
			return LOCTEXT("Workbench", "À l'établi");
		}
		if (Station == TEXT("forge"))
		{
			return LOCTEXT("Forge", "À la forge");
		}
		return LOCTEXT("Hand", "À la main");
	}

	/** Réponse du serveur à une fabrication, en clair. */
	FText OutcomeText(EOdysseeCraftOutcome Outcome)
	{
		switch (Outcome)
		{
		case EOdysseeCraftOutcome::Done: return LOCTEXT("Done", "Fabriqué !");
		case EOdysseeCraftOutcome::Busy: return LOCTEXT("Busy", "Une fabrication est déjà en cours.");
		case EOdysseeCraftOutcome::NeedStation: return LOCTEXT("NeedStation", "Il faut être près de la bonne station.");
		case EOdysseeCraftOutcome::MissingIngredients: return LOCTEXT("Missing", "Il manque des ingrédients.");
		case EOdysseeCraftOutcome::UnknownRecipe: return LOCTEXT("Unknown", "Recette inconnue du serveur.");
		default: return FText::GetEmpty();
		}
	}
}

void AOdyssePlayerController::CreatePanels()
{
	InventoryPanel = CreateWidget<UOdysseeInventoryWidget>(this);
	InventoryPanel->OnMoveRequested.BindUObject(this, &AOdyssePlayerController::RequestMove);
	CraftingPanel = CreateWidget<UOdysseeCraftingWidget>(this);
	CraftingPanel->OnCraftRequested.BindUObject(this, &AOdyssePlayerController::RequestCraft);
	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->OnCraftChanged.AddUniqueDynamic(this, &AOdyssePlayerController::OnCraftChanged);
	}
}

void AOdyssePlayerController::ToggleInventory()
{
	if (!bInGame || PauseMenu->IsInViewport())
	{
		return;
	}
	if (InventoryPanel->IsInViewport())
	{
		ClosePanels();
		return;
	}
	ClosePanels(); // un seul panneau à la fois
	OpenPanel(InventoryPanel);
	UpdateInventoryPanel();
}

void AOdyssePlayerController::ToggleCrafting()
{
	if (!bInGame || PauseMenu->IsInViewport())
	{
		return;
	}
	if (CraftingPanel->IsInViewport())
	{
		ClosePanels();
		return;
	}
	ClosePanels();
	TArray<FOdysseeRecipeView> Views;
	if (const UOdysseeNetSubsystem* Net = GetNet())
	{
		for (const FOdysseeRecipe& Recipe : Net->GetRecipes())
		{
			Views.Add({Recipe.Key, Recipe.Name, StationName(Recipe.Station), Recipe.Ingredients, Recipe.Seconds});
		}
	}
	CraftingPanel->ShowRecipes(Views);
	OpenPanel(CraftingPanel);
}

void AOdyssePlayerController::OpenPanel(UUserWidget* Panel)
{
	Panel->AddToViewport(5);
	// Le clavier reste au jeu (on marche, Tab et C referment) ; la souris sert au panneau.
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
	SetIgnoreLookInput(true);
}

bool AOdyssePlayerController::ClosePanels()
{
	bool bClosed = false;
	for (UUserWidget* Panel : {static_cast<UUserWidget*>(InventoryPanel), static_cast<UUserWidget*>(CraftingPanel)})
	{
		if (Panel && Panel->IsInViewport())
		{
			Panel->RemoveFromParent();
			bClosed = true;
		}
	}
	if (bClosed)
	{
		SetGameInput();
	}
	return bClosed;
}

void AOdyssePlayerController::UpdateInventoryPanel()
{
	if (!InventoryPanel || !InventoryPanel->IsInViewport())
	{
		return; // fermé : rien à tenir à jour (il se remplit à l'ouverture)
	}
	const UOdysseeNetSubsystem* Net = GetNet();
	const TArray<FOdysseeInventorySlot> Inventory = Net && Net->IsConnected()
		? Net->GetInventory() : TArray<FOdysseeInventorySlot>();
	TArray<FOdysseeSlotView> Views;
	Views.SetNum(UOdysseeInventoryWidget::SlotCount);
	for (int32 Index = 0; Index < Views.Num() && Index < Inventory.Num(); ++Index)
	{
		const FOdysseeInventorySlot& Cell = Inventory[Index];
		if (Cell.Count > 0)
		{
			Views[Index] = {IconFor(Cell.IconName), Cell.Count, Cell.Name};
		}
	}
	InventoryPanel->ShowSlots(Views);
}

void AOdyssePlayerController::RequestMove(int32 From, int32 To)
{
	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->RequestMoveItem(From, To); // le serveur vérifie, puis renvoie l'inventaire
	}
}

void AOdyssePlayerController::RequestCraft(FName Recipe)
{
	UOdysseeNetSubsystem* Net = GetNet();
	if (!Net || !Net->IsConnected())
	{
		CraftingPanel->ShowCraftMessage(LOCTEXT("CraftOffline", "Hors ligne : la fabrication se fait sur le serveur."),
			false);
		return;
	}
	Net->RequestCraft(Recipe);
}

void AOdyssePlayerController::OnCraftChanged(FName RecipeKey, EOdysseeCraftOutcome Outcome, float Seconds)
{
	if (Outcome == EOdysseeCraftOutcome::Started)
	{
		CraftingPanel->ShowCraftStarted(RecipeKey, Seconds);
		return;
	}
	CraftingPanel->ShowCraftMessage(OutcomeText(Outcome), Outcome == EOdysseeCraftOutcome::Done);
	if (Outcome == EOdysseeCraftOutcome::Done)
	{
		Hud->AddNotification(LOCTEXT("Crafted", "Objet fabriqué")); // visible même panneau fermé
	}
}

#undef LOCTEXT_NAMESPACE
