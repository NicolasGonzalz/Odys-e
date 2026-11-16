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

	/** Nom d'onglet d'une catégorie d'objet (data/items.txt, `category`). */
	FText CategoryLabel(FName Category)
	{
		static const TMap<FName, FText> Labels = {
			{TEXT("tool"), LOCTEXT("Tools", "Outils")},
			{TEXT("weapon"), LOCTEXT("Weapons", "Armes")},
			{TEXT("ammo"), LOCTEXT("Ammo", "Munitions")},
			{TEXT("resource"), LOCTEXT("Resources", "Matériaux")},
			{TEXT("station"), LOCTEXT("Stations", "Stations")},
			{TEXT("consumable"), LOCTEXT("Consumables", "Consommables")},
			{TEXT("equipment"), LOCTEXT("Equipment", "Équipement")},
		};
		const FText* Label = Labels.Find(Category);
		return Label ? *Label : FText::FromName(Category);
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
	ToggleStation(TEXT("hand")); // C : seulement ce qui se fabrique à la main, partout
}

void AOdyssePlayerController::OdysseeStation(const FString& Which)
{
	const FString Name = Which.ToLower();
	ToggleStation(Name == TEXT("forge") ? FName(TEXT("forge"))
		: Name == TEXT("etabli") || Name == TEXT("workbench") ? FName(TEXT("workbench")) : FName(TEXT("hand")));
}

void AOdyssePlayerController::ToggleStation(FName Station)
{
	if (!bInGame || PauseMenu->IsInViewport())
	{
		return;
	}
	const bool bSameOpen = CraftingPanel->IsInViewport() && OpenStationName == Station;
	ClosePanels();
	if (bSameOpen)
	{
		return; // même touche : on referme
	}
	OpenStationName = Station;
	TArray<FOdysseeRecipeView> Views;
	if (const UOdysseeNetSubsystem* Net = GetNet())
	{
		const auto ItemView = [this](const FOdysseeIngredient& Item) {
			return FOdysseeItemView{Item.ItemKey, IconFor(Item.IconName), Item.Name, Item.Count};
		};
		for (const FOdysseeRecipe& Recipe : Net->GetRecipes())
		{
			// À la main : partout (touche C, et aussi à l'établi et à la forge). Une recette d'établi ou
			// de forge n'apparaît qu'à sa station.
			if (Recipe.Station != Station && Recipe.Station != TEXT("hand"))
			{
				continue;
			}
			FOdysseeRecipeView& View = Views.AddDefaulted_GetRef();
			View.Key = Recipe.Key;
			View.Category = Recipe.Category;
			View.CategoryLabel = CategoryLabel(Recipe.Category);
			View.Station = StationName(Recipe.Station);
			View.Seconds = Recipe.Seconds;
			View.Output = ItemView(Recipe.Output);
			for (const FOdysseeIngredient& Input : Recipe.Inputs)
			{
				View.Inputs.Add(ItemView(Input));
			}
		}
	}
	CraftingPanel->SetTitle(Station == TEXT("forge") ? LOCTEXT("ForgeTitle", "Forge")
		: Station == TEXT("workbench") ? LOCTEXT("WorkbenchTitle", "Établi") : LOCTEXT("HandTitle", "Artisanat"));
	CraftingPanel->ShowRecipes(Views);
	OpenPanel(CraftingPanel);
	UpdateCraftingPanel();
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

void AOdyssePlayerController::UpdateCraftingPanel()
{
	if (!CraftingPanel || !CraftingPanel->IsInViewport())
	{
		return;
	}
	// Ce que le joueur possède, objet par objet (plusieurs cases peuvent tenir le même objet).
	TMap<FName, int32> Owned;
	const UOdysseeNetSubsystem* Net = GetNet();
	if (Net && Net->IsConnected())
	{
		for (const FOdysseeInventorySlot& Cell : Net->GetInventory())
		{
			if (Cell.Count > 0)
			{
				Owned.FindOrAdd(Cell.ItemKey) += Cell.Count;
			}
		}
	}
	CraftingPanel->ShowOwned(Owned);
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
