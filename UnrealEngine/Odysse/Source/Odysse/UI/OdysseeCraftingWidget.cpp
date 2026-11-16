#include "UI/OdysseeCraftingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/OdysseeUiStyle.h"

#define LOCTEXT_NAMESPACE "OdysseeCrafting"

namespace
{
	constexpr float ListWidth = 360.f;
	constexpr float DetailWidth = 380.f;
	const FLinearColor Success(0.55f, 0.82f, 0.42f);
}

// --- Une recette de la liste ---------------------------------------------------------------------

UWidget* UOdysseeRecipeEntryWidget::BuildTree()
{
	Button = MakeButton(TEXT("RecipeButton"), FText::GetEmpty(), ListWidth - 20.f);
	Label = Cast<UTextBlock>(Button->GetChildAt(0));
	Button->OnClicked.AddDynamic(this, &UOdysseeRecipeEntryWidget::HandleClicked);
	return Button->GetParent(); // la boîte à la taille du bouton
}

void UOdysseeRecipeEntryWidget::Show(int32 InIndex, const FText& Text)
{
	Index = InIndex;
	Label->SetText(Text.ToUpper());
}

void UOdysseeRecipeEntryWidget::SetSelected(bool bSelected)
{
	Label->SetColorAndOpacity(FSlateColor(bSelected ? OdysseeUi::Rust : OdysseeUi::Bone));
}

void UOdysseeRecipeEntryWidget::HandleClicked()
{
	OnChosen.ExecuteIfBound(Index);
}

// --- L'écran ------------------------------------------------------------------------------------

UWidget* UOdysseeCraftingWidget::BuildTree()
{
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UHorizontalBox* Panels = MakeRow();
	AddTo(Panels, BuildList(), FMargin(0.f, 0.f, 16.f, 0.f), VAlign_Top);
	AddTo(Panels, BuildDetail(), FMargin(0.f), VAlign_Top);

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
	RecipeList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RecipeList"));
	AddTo(Column, MakeSized(RecipeList, FVector2D(ListWidth, 520.f)), FMargin(0.f));
	return Panel;
}

UWidget* UOdysseeCraftingWidget::BuildDetail()
{
	RecipeName = MakeText(TEXT("RecipeName"), LOCTEXT("Choose", "Choisissez une recette"), 20, true);
	RecipeName->SetAutoWrapText(true);
	StationText = MakeText(TEXT("StationText"), FText::GetEmpty(), 14, true, OdysseeUi::Rust);
	IngredientsText = MakeText(TEXT("IngredientsText"), FText::GetEmpty(), 15);
	IngredientsText->SetAutoWrapText(true);
	DurationText = MakeText(TEXT("DurationText"), FText::GetEmpty(), 13, false, OdysseeUi::BoneDim);
	CraftButton = MakeButton(TEXT("CraftButton"), LOCTEXT("Craft", "Fabriquer"), 260.f);
	CraftButton->OnClicked.AddDynamic(this, &UOdysseeCraftingWidget::HandleCraftClicked);
	CraftButton->SetIsEnabled(false);
	CraftProgress = MakeBar(TEXT("CraftProgress"), OdysseeUi::Stamina, 0.f);
	StatusText = MakeText(TEXT("StatusText"), FText::GetEmpty(), 14, true);
	StatusText->SetAutoWrapText(true);

	UVerticalBox* Column = MakeColumn();
	AddTo(Column, RecipeName, FMargin(0.f, 0.f, 0.f, 4.f), HAlign_Left);
	AddTo(Column, StationText, FMargin(0.f, 0.f, 0.f, 16.f), HAlign_Left);
	AddTo(Column, MakeText(NAME_None, LOCTEXT("Needs", "Il faut :"), 13, true, OdysseeUi::BoneDim),
		FMargin(0.f, 0.f, 0.f, 2.f), HAlign_Left);
	AddTo(Column, IngredientsText, FMargin(0.f, 0.f, 0.f, 8.f), HAlign_Left);
	AddTo(Column, DurationText, FMargin(0.f, 0.f, 0.f, 22.f), HAlign_Left);
	AddButton(Column, CraftButton);
	AddTo(Column, MakeSized(CraftProgress, FVector2D(260.f, 12.f)), FMargin(0.f, 10.f), HAlign_Center);
	AddTo(Column, StatusText, FMargin(0.f, 4.f, 0.f, 0.f), HAlign_Center);
	CraftProgress->GetParent()->SetVisibility(ESlateVisibility::Hidden);
	return MakePanel(TEXT("RecipeDetail"), MakeSized(Column, FVector2D(DetailWidth, 420.f)), FMargin(26.f));
}

void UOdysseeCraftingWidget::ShowRecipes(const TArray<FOdysseeRecipeView>& Recipes)
{
	Shown = Recipes;
	RecipeList->ClearChildren();
	Entries.Reset();
	for (int32 Index = 0; Index < Recipes.Num(); ++Index)
	{
		UOdysseeRecipeEntryWidget* Entry = WidgetTree->ConstructWidget<UOdysseeRecipeEntryWidget>(
			UOdysseeRecipeEntryWidget::StaticClass());
		Entry->Show(Index, Recipes[Index].Name);
		Entry->OnChosen.BindUObject(this, &UOdysseeCraftingWidget::Select);
		RecipeList->AddChild(Entry);
		Entries.Add(Entry);
	}
	Select(Recipes.IsValidIndex(Selected) ? Selected : (Recipes.Num() > 0 ? 0 : -1));
}

void UOdysseeCraftingWidget::Select(int32 Index)
{
	Selected = Index;
	for (int32 Entry = 0; Entry < Entries.Num(); ++Entry)
	{
		Entries[Entry]->SetSelected(Entry == Index);
	}
	const bool bValid = Shown.IsValidIndex(Index);
	CraftButton->SetIsEnabled(bValid && CraftDuration <= 0.f);
	if (!bValid)
	{
		return;
	}
	const FOdysseeRecipeView& Recipe = Shown[Index];
	RecipeName->SetText(Recipe.Name);
	StationText->SetText(Recipe.Station);
	IngredientsText->SetText(Recipe.Ingredients);
	DurationText->SetText(FText::Format(LOCTEXT("Duration", "Durée : {0} s"),
		FText::AsNumber(FMath::RoundToInt(Recipe.Seconds))));
	StatusText->SetText(FText::GetEmpty());
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
	StatusText->SetColorAndOpacity(FSlateColor(OdysseeUi::Bone));
	StatusText->SetText(LOCTEXT("Crafting", "Fabrication…"));
}

void UOdysseeCraftingWidget::ShowCraftMessage(const FText& Message, bool bSuccess)
{
	CraftDuration = 0.f;
	CraftProgress->GetParent()->SetVisibility(ESlateVisibility::Hidden);
	CraftButton->SetIsEnabled(Shown.IsValidIndex(Selected));
	StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? Success : OdysseeUi::Health));
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
