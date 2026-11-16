#include "UI/OdysseeCraftingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "UI/OdysseeUiStyle.h"

#define LOCTEXT_NAMESPACE "OdysseeCrafting"

namespace
{
	constexpr int32 Columns = 5;
	const FVector2D CardSize(132.f, 160.f);
	const FVector2D TabSize(120.f, 40.f);
	const float GridWidth = Columns * (CardSize.X + 10.f) + 12.f;
	constexpr float CardIcon = 80.f;
	constexpr float TileIcon = 60.f;
	constexpr float DetailWidth = 560.f;

	/** Taille d'une case de la formule : plus il y a d'objets, plus elles sont petites (une ligne). */
	float TileSizeFor(int32 Items)
	{
		return Items <= 3 ? 84.f : (Items == 4 ? 72.f : 60.f);
	}
	const FLinearColor Enough(0.55f, 0.82f, 0.42f);

	/** Carte de recette : la case facettée de l'inventaire, allumée au survol ou si choisie. */
	FButtonStyle CardStyle(bool bSelected, EOdysseeUiTheme Theme)
	{
		const FMargin Margin(0.2f);
		const FSlateBrush Normal = OdysseeUi::BoxBrush(bSelected ? TEXT("case_active") : TEXT("case"), Margin, Theme);
		const FSlateBrush Lit = OdysseeUi::BoxBrush(TEXT("case_active"), Margin, Theme);
		FButtonStyle Style;
		Style.SetNormal(Normal).SetHovered(Lit).SetPressed(Lit).SetDisabled(Normal);
		Style.SetNormalPadding(FMargin(6.f)).SetPressedPadding(FMargin(6.f, 7.f, 6.f, 5.f));
		return Style;
	}
}

// --- Carte de recette ou onglet ------------------------------------------------------------------

UWidget* UOdysseeRecipeEntryWidget::BuildTree()
{
	Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EntryButton"));
	Picture = MakeImage(TEXT("Picture"), TEXT("outil_baton"), FVector2D(CardIcon));
	Label = MakeText(TEXT("Label"), FText::GetEmpty(), 12, true);
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	UVerticalBox* Column = MakeColumn();
	AddTo(Column, Picture, FMargin(0.f, 4.f, 0.f, 6.f), HAlign_Center);
	AddTo(Column, Label, FMargin(0.f), HAlign_Center);
	Button->AddChild(Column);
	Button->OnClicked.AddDynamic(this, &UOdysseeRecipeEntryWidget::HandleClicked);
	return MakeSized(Button, CardSize);
}

void UOdysseeRecipeEntryWidget::Show(int32 InIndex, const FText& Text, const FString& Icon, bool bTab)
{
	Index = InIndex;
	bIsTab = bTab;
	USizeBox* Box = Cast<USizeBox>(Button->GetParent());
	if (bTab)
	{
		Box->SetWidthOverride(TabSize.X);
		Box->SetHeightOverride(TabSize.Y);
		Button->SetStyle(OdysseeUi::ButtonStyle(Theme));
		Label->SetText(Text.ToUpper());
		Picture->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Button->SetStyle(CardStyle(false, Theme));
	Label->SetText(Text);
	Picture->SetVisibility(Icon.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	if (!Icon.IsEmpty())
	{
		Picture->SetBrush(OdysseeUi::ImageBrush(*Icon, FVector2D(CardIcon), Theme));
		Picture->SetColorAndOpacity(IconTint());
	}
}

void UOdysseeRecipeEntryWidget::SetSelected(bool bSelected)
{
	Label->SetColorAndOpacity(FSlateColor(bSelected ? AccentColor() : TextColor()));
	if (!bIsTab)
	{
		Button->SetStyle(CardStyle(bSelected, Theme));
	}
}

void UOdysseeRecipeEntryWidget::HandleClicked()
{
	OnChosen.ExecuteIfBound(Index);
}

// --- L'écran ------------------------------------------------------------------------------------

UWidget* UOdysseeCraftingWidget::BuildTree()
{
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, Theme == EOdysseeUiTheme::Holo ? 0.f : 0.45f));
	UHorizontalBox* Panels = MakeRow();
	AddTo(Panels, BuildList(), FMargin(0.f, 0.f, 16.f, 0.f), VAlign_Fill);
	AddTo(Panels, BuildDetail(), FMargin(0.f), VAlign_Fill);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* Back = Root->AddChildToOverlay(Veil);
	Back->SetHorizontalAlignment(HAlign_Fill);
	Back->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* Front = Root->AddChildToOverlay(Panels);
	Front->SetHorizontalAlignment(HAlign_Center);
	Front->SetVerticalAlignment(VAlign_Center);
	return Root;
}

UWidget* UOdysseeCraftingWidget::BuildList()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("Title", "Artisanat"), Column);
	// Le titre est le premier élément du panneau (MakeMenuPanel) : on le garde pour le changer.
	TitleText = Cast<UTextBlock>(Cast<UPanelWidget>(Panel->GetContent())->GetChildAt(0));
	// Onglets à la ligne quand ils ne tiennent pas : le panneau garde la largeur de la grille.
	CategoryTabs = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("CategoryTabs"));
	CategoryTabs->SetExplicitWrapSize(true);
	CategoryTabs->SetWrapSize(GridWidth);
	CategoryTabs->SetInnerSlotPadding(FVector2D(6.f));
	AddTo(Column, CategoryTabs, FMargin(0.f, 0.f, 0.f, 14.f), HAlign_Left);
	RecipeGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("RecipeGrid"));
	RecipeGrid->SetSlotPadding(FMargin(5.f));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RecipeScroll"));
	Scroll->AddChild(RecipeGrid);
	AddTo(Column, MakeSized(Scroll, FVector2D(GridWidth, 500.f)), FMargin(0.f));
	return Panel;
}

UWidget* UOdysseeCraftingWidget::BuildDetail()
{
	OutputIcon = MakeImage(TEXT("OutputIcon"), TEXT("outil_baton"), FVector2D(128.f));
	RecipeName = MakeText(TEXT("RecipeName"), LOCTEXT("Choose", "Choisissez une recette"), 22, true);
	RecipeName->SetAutoWrapText(true);
	StationText = MakeText(TEXT("StationText"), FText::GetEmpty(), 14, true, AccentColor());
	Formula = MakeRow(TEXT("Formula"));
	DurationText = MakeText(TEXT("DurationText"), FText::GetEmpty(), 13, false, DimColor());
	CraftButton = MakeButton(TEXT("CraftButton"), LOCTEXT("Craft", "Fabriquer"), 300.f);
	CraftButton->OnClicked.AddDynamic(this, &UOdysseeCraftingWidget::HandleCraftClicked);
	CraftButton->SetIsEnabled(false);
	CraftProgress = MakeBar(TEXT("CraftProgress"), OdysseeUi::Stamina, 0.f);
	StatusText = MakeText(TEXT("StatusText"), FText::GetEmpty(), 14, true);
	StatusText->SetAutoWrapText(true);
	UTextBlock* Needs = MakeText(NAME_None, LOCTEXT("Needs", "INGRÉDIENTS"), 11, true, DimColor());
	Needs->SetFont(OdysseeUi::Font(11, true, 250));

	UVerticalBox* Column = MakeColumn();
	AddTo(Column, MakeSized(OutputIcon, FVector2D(128.f)), FMargin(0.f, 0.f, 0.f, 8.f), HAlign_Center);
	AddTo(Column, RecipeName, FMargin(0.f), HAlign_Center);
	AddTo(Column, StationText, FMargin(0.f, 2.f, 0.f, 18.f), HAlign_Center);
	AddTo(Column, Needs, FMargin(0.f, 0.f, 0.f, 8.f), HAlign_Left);
	AddTo(Column, Formula, FMargin(0.f, 0.f, 0.f, 12.f), HAlign_Center);
	AddTo(Column, DurationText, FMargin(0.f, 0.f, 0.f, 18.f), HAlign_Center);
	AddButton(Column, CraftButton);
	AddTo(Column, MakeSized(CraftProgress, FVector2D(300.f, 12.f)), FMargin(0.f, 10.f), HAlign_Center);
	AddTo(Column, StatusText, FMargin(0.f, 4.f, 0.f, 0.f), HAlign_Center);
	CraftProgress->GetParent()->SetVisibility(ESlateVisibility::Hidden);
	OutputIcon->SetVisibility(ESlateVisibility::Hidden);
	return MakePanel(TEXT("RecipeDetail"), MakeSized(Column, FVector2D(DetailWidth, 640.f)), FMargin(28.f));
}

void UOdysseeCraftingWidget::SetTitle(const FText& Title)
{
	TitleText->SetText(Title.ToUpper());
}

void UOdysseeCraftingWidget::ShowRecipes(const TArray<FOdysseeRecipeView>& Recipes)
{
	Shown = Recipes; // nouvelle liste (autre station) : on repart de « Tout » et de la première
	Selected = -1;
	BuildTabs();
	SelectTab(0);
}

void UOdysseeCraftingWidget::BuildTabs()
{
	// « Tout », puis une catégorie par type d'objet fabriqué, dans l'ordre où elles apparaissent.
	Tabs = {NAME_None};
	TArray<FText> Labels = {LOCTEXT("All", "Tout")};
	for (const FOdysseeRecipeView& Recipe : Shown)
	{
		if (!Tabs.Contains(Recipe.Category))
		{
			Tabs.Add(Recipe.Category);
			Labels.Add(Recipe.CategoryLabel);
		}
	}
	CategoryTabs->ClearChildren();
	TabEntries.Reset();
	for (int32 Tab = 0; Tab < Tabs.Num(); ++Tab)
	{
		UOdysseeRecipeEntryWidget* Entry = MakeChild<UOdysseeRecipeEntryWidget>();
		Entry->Show(Tab, Labels[Tab], FString(), true);
		Entry->OnChosen.BindUObject(this, &UOdysseeCraftingWidget::SelectTab);
		CategoryTabs->AddChildToWrapBox(Entry);
		TabEntries.Add(Entry);
	}
}

void UOdysseeCraftingWidget::SelectTab(int32 Tab)
{
	SelectedTab = Tab;
	for (int32 Entry = 0; Entry < TabEntries.Num(); ++Entry)
	{
		TabEntries[Entry]->SetSelected(Entry == Tab);
	}
	FillGrid();
	const bool bStillShown = CardRecipes.Contains(Selected);
	Select(bStillShown ? Selected : (CardRecipes.Num() > 0 ? CardRecipes[0] : -1));
}

void UOdysseeCraftingWidget::FillGrid()
{
	RecipeGrid->ClearChildren();
	Cards.Reset();
	CardRecipes.Reset();
	const FName Category = Tabs.IsValidIndex(SelectedTab) ? Tabs[SelectedTab] : NAME_None;
	for (int32 Recipe = 0; Recipe < Shown.Num(); ++Recipe)
	{
		if (Category != NAME_None && Shown[Recipe].Category != Category)
		{
			continue;
		}
		const int32 Card = Cards.Num();
		UOdysseeRecipeEntryWidget* Entry = MakeChild<UOdysseeRecipeEntryWidget>();
		Entry->Show(Card, Shown[Recipe].Output.Name, Shown[Recipe].Output.Icon, false);
		Entry->OnChosen.BindUObject(this, &UOdysseeCraftingWidget::ChooseCard);
		RecipeGrid->AddChildToUniformGrid(Entry, Card / Columns, Card % Columns);
		Cards.Add(Entry);
		CardRecipes.Add(Recipe);
	}
}

void UOdysseeCraftingWidget::ChooseCard(int32 Card)
{
	Select(CardRecipes.IsValidIndex(Card) ? CardRecipes[Card] : -1);
}

void UOdysseeCraftingWidget::Select(int32 Recipe)
{
	Selected = Recipe;
	for (int32 Card = 0; Card < Cards.Num(); ++Card)
	{
		Cards[Card]->SetSelected(CardRecipes[Card] == Recipe);
	}
	const bool bValid = Shown.IsValidIndex(Recipe);
	CraftButton->SetIsEnabled(bValid && CraftDuration <= 0.f);
	OutputIcon->SetVisibility(bValid && !Shown[Recipe].Output.Icon.IsEmpty() ? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Hidden);
	if (!bValid)
	{
		Formula->ClearChildren();
		return;
	}
	const FOdysseeRecipeView& View = Shown[Recipe];
	if (!View.Output.Icon.IsEmpty())
	{
		OutputIcon->SetBrush(OdysseeUi::ImageBrush(*View.Output.Icon, FVector2D(128.f), Theme));
		OutputIcon->SetColorAndOpacity(IconTint());
	}
	RecipeName->SetText(View.Output.Name);
	StationText->SetText(View.Station);
	DurationText->SetText(FText::Format(LOCTEXT("Duration", "Durée : {0} s"),
		FText::AsNumber(FMath::RoundToInt(View.Seconds))));
	StatusText->SetText(FText::GetEmpty());
	ShowFormula(View);
}

void UOdysseeCraftingWidget::ShowFormula(const FOdysseeRecipeView& Recipe)
{
	Formula->ClearChildren();
	const float Size = TileSizeFor(Recipe.Inputs.Num() + 1);
	const FMargin SignSpace(4.f, Size * 0.3f, 4.f, 0.f); // signes à mi-hauteur des cases
	for (int32 Input = 0; Input < Recipe.Inputs.Num(); ++Input)
	{
		if (Input > 0)
		{
			AddTo(Formula, MakeText(NAME_None, FText::FromString(TEXT("+")), 22, true, DimColor()),
				SignSpace, VAlign_Top);
		}
		AddTo(Formula, MakeItemTile(Recipe.Inputs[Input], true, Size), FMargin(0.f), VAlign_Top);
	}
	AddTo(Formula, MakeText(NAME_None, FText::FromString(TEXT("→")), 22, true, AccentColor()),
		FMargin(8.f, Size * 0.3f, 8.f, 0.f), VAlign_Top);
	AddTo(Formula, MakeItemTile(Recipe.Output, false, Size), FMargin(0.f), VAlign_Top);
}

UWidget* UOdysseeCraftingWidget::MakeItemTile(const FOdysseeItemView& Item, bool bIngredient, float Size)
{
	UOverlay* Frame = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* Back = Frame->AddChildToOverlay(MakeImage(NAME_None, TEXT("case"), FVector2D(Size)));
	Back->SetHorizontalAlignment(HAlign_Center);
	if (!Item.Icon.IsEmpty())
	{
		UOverlaySlot* Front = Frame->AddChildToOverlay(MakeImage(NAME_None, *Item.Icon, FVector2D(Size * TileIcon / 84.f), IconTint()));
		Front->SetHorizontalAlignment(HAlign_Center);
		Front->SetVerticalAlignment(VAlign_Center);
	}
	UVerticalBox* Tile = MakeColumn();
	AddTo(Tile, MakeSized(Frame, FVector2D(Size)), FMargin(0.f), HAlign_Center);
	AddTo(Tile, MakeText(NAME_None, FText::Format(LOCTEXT("Times", "×{0}"), Item.Count), 16, true),
		FMargin(0.f, 4.f, 0.f, 0.f), HAlign_Center);
	if (bIngredient)
	{
		// Ce qu'on a : vert si assez, rouge sinon (le serveur vérifie de toute façon).
		const int32 Have = OwnedItems.FindRef(Item.Key);
		UTextBlock* Owned = MakeText(NAME_None, FText::Format(LOCTEXT("Owned", "{0} / {1}"), Have, Item.Count), 13,
			true, Have >= Item.Count ? Enough : OdysseeUi::Health);
		AddTo(Tile, Owned, FMargin(0.f, 2.f, 0.f, 0.f), HAlign_Center);
	}
	return Tile;
}

void UOdysseeCraftingWidget::ShowOwned(const TMap<FName, int32>& Owned)
{
	if (Owned.OrderIndependentCompareEqual(OwnedItems))
	{
		return; // inchangé : la formule reste telle quelle
	}
	OwnedItems = Owned;
	if (Shown.IsValidIndex(Selected))
	{
		ShowFormula(Shown[Selected]);
	}
}

void UOdysseeCraftingWidget::HandleCraftClicked()
{
	if (Shown.IsValidIndex(Selected))
	{
		OnCraftRequested.ExecuteIfBound(Shown[Selected].Key);
	}
}

void UOdysseeCraftingWidget::ShowCraftStarted(FName Recipe, float Seconds)
{
	CraftElapsed = 0.f;
	CraftDuration = FMath::Max(Seconds, 0.01f);
	CraftProgress->SetPercent(0.f);
	CraftProgress->GetParent()->SetVisibility(ESlateVisibility::HitTestInvisible);
	CraftButton->SetIsEnabled(false); // une fabrication à la fois
	StatusText->SetColorAndOpacity(FSlateColor(TextColor()));
	StatusText->SetText(LOCTEXT("Crafting", "Fabrication…"));
}

void UOdysseeCraftingWidget::ShowCraftMessage(const FText& Message, bool bSuccess)
{
	CraftDuration = 0.f;
	CraftProgress->GetParent()->SetVisibility(ESlateVisibility::Hidden);
	CraftButton->SetIsEnabled(Shown.IsValidIndex(Selected));
	StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? Enough : OdysseeUi::Health));
	StatusText->SetText(Message);
}

void UOdysseeCraftingWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (CraftDuration > 0.f)
	{
		// Estimation locale : la fin officielle arrive par le serveur (ShowCraftMessage).
		CraftElapsed = FMath::Min(CraftElapsed + DeltaTime, CraftDuration);
		CraftProgress->SetPercent(CraftElapsed / CraftDuration);
	}
}

#undef LOCTEXT_NAMESPACE
