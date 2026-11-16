// Le bracelet de stase (docs/BRACELET.md) : Tab l'ouvre sur le dernier onglet, I, C, J et M
// directement sur Sac, Fabrication, Histoire et Carte (même touche pour refermer). Il s'ouvre
// par-dessus le jeu sans jamais le mettre en pause : on peut continuer à marcher.

#include "OdyssePlayerController.h"

#include "UI/OdysseeBraceletWidget.h"
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
	Bracelet = CreateWidget<UOdysseeBraceletWidget>(this);
	Bracelet->Inventory->OnMoveRequested.BindUObject(this, &AOdyssePlayerController::RequestMove);
	Bracelet->Crafting->OnCraftRequested.BindUObject(this, &AOdyssePlayerController::RequestCraft);
	if (UOdysseeNetSubsystem* Net = GetNet())
	{
		Net->OnCraftChanged.AddUniqueDynamic(this, &AOdyssePlayerController::OnCraftChanged);
	}
}

bool AOdyssePlayerController::CanUseBracelet() const
{
	return bInGame && !PauseMenu->IsInViewport();
}

void AOdyssePlayerController::ToggleBracelet()
{
	if (!CanUseBracelet())
	{
		return;
	}
	if (ClosePanels())
	{
		return; // Tab referme, quel que soit l'onglet
	}
	if (LastTab == EOdysseeBraceletTab::Crafting)
	{
		FillCrafting(OpenStationName.IsNone() ? FName(TEXT("hand")) : OpenStationName);
	}
	OpenBracelet(LastTab); // Tab : le dernier onglet ouvert
}

void AOdyssePlayerController::ToggleTab(EOdysseeBraceletTab Tab)
{
	if (!CanUseBracelet())
	{
		return;
	}
	if (Bracelet->IsInViewport() && Bracelet->GetTab() == Tab)
	{
		ClosePanels(); // même raccourci : on referme
		return;
	}
	OpenBracelet(Tab);
}

void AOdyssePlayerController::OpenBagTab() { ToggleTab(EOdysseeBraceletTab::Bag); }
void AOdyssePlayerController::OpenStoryTab() { ToggleTab(EOdysseeBraceletTab::Story); }
void AOdyssePlayerController::OpenMapTab() { ToggleTab(EOdysseeBraceletTab::Map); }

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
	if (!CanUseBracelet())
	{
		return;
	}
	const bool bSameOpen = Bracelet->IsInViewport() && Bracelet->GetTab() == EOdysseeBraceletTab::Crafting
		&& OpenStationName == Station;
	if (bSameOpen)
	{
		ClosePanels(); // même touche : on referme
		return;
	}
	FillCrafting(Station);
	OpenBracelet(EOdysseeBraceletTab::Crafting);
}

void AOdyssePlayerController::FillCrafting(FName Station)
{
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
	Bracelet->Crafting->SetTitle(Station == TEXT("forge") ? LOCTEXT("ForgeTitle", "Forge")
		: Station == TEXT("workbench") ? LOCTEXT("WorkbenchTitle", "Établi") : LOCTEXT("HandTitle", "Fabrication"));
	Bracelet->Crafting->ShowRecipes(Views);
}

void AOdyssePlayerController::OpenBracelet(EOdysseeBraceletTab Tab)
{
	LastTab = Tab;
	Bracelet->ShowTab(Tab);
	if (!Bracelet->IsInViewport())
	{
		Bracelet->AddToViewport(5);
		Bracelet->PlayOpen();
		// Le clavier reste au jeu (on marche, les raccourcis referment) ; la souris sert au bracelet.
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
		SetIgnoreLookInput(true);
	}
	UpdateInventoryPanel();
	UpdateCraftingPanel();
}

bool AOdyssePlayerController::ClosePanels()
{
	if (!Bracelet || !Bracelet->IsInViewport())
	{
		return false;
	}
	LastTab = Bracelet->GetTab();
	Bracelet->RemoveFromParent();
	SetGameInput();
	return true;
}

void AOdyssePlayerController::UpdateInventoryPanel()
{
	if (!Bracelet || !Bracelet->IsInViewport() || Bracelet->GetTab() != EOdysseeBraceletTab::Bag)
	{
		return; // pas à l'écran : rien à tenir à jour (il se remplit à l'ouverture)
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
	Bracelet->Inventory->ShowSlots(Views);
}

void AOdyssePlayerController::UpdateCraftingPanel()
{
	if (!Bracelet || !Bracelet->IsInViewport() || Bracelet->GetTab() != EOdysseeBraceletTab::Crafting)
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
	Bracelet->Crafting->ShowOwned(Owned);
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
		Bracelet->Crafting->ShowCraftMessage(LOCTEXT("CraftOffline", "Hors ligne : la fabrication se fait sur le serveur."),
			false);
		return;
	}
	Net->RequestCraft(Recipe);
}

void AOdyssePlayerController::OnCraftChanged(FName RecipeKey, EOdysseeCraftOutcome Outcome, float Seconds)
{
	if (Outcome == EOdysseeCraftOutcome::Started)
	{
		Bracelet->Crafting->ShowCraftStarted(RecipeKey, Seconds);
		return;
	}
	Bracelet->Crafting->ShowCraftMessage(OutcomeText(Outcome), Outcome == EOdysseeCraftOutcome::Done);
	if (Outcome == EOdysseeCraftOutcome::Done)
	{
		Hud->AddNotification(LOCTEXT("Crafted", "Objet fabriqué")); // visible même panneau fermé
	}
}

#undef LOCTEXT_NAMESPACE
