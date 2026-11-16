#include "UI/OdysseeInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "UI/OdysseeUiStyle.h"

#define LOCTEXT_NAMESPACE "OdysseeInventory"

namespace
{
	constexpr int32 Columns = 5;
	constexpr float DetailWidth = 300.f;
	constexpr float DetailIconSize = 140.f;
}

UWidget* UOdysseeInventoryWidget::BuildTree()
{
	// Le jeu reste visible derrière un voile léger : il continue pendant qu'on range.
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	// Dans le bracelet (hologramme), pas de voile : c'est le bracelet qui décide du fond.
	Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, Theme == EOdysseeUiTheme::Holo ? 0.f : 0.45f));
	UHorizontalBox* Panels = MakeRow();
	AddTo(Panels, BuildGrid(), FMargin(0.f, 0.f, 16.f, 0.f), VAlign_Top);
	AddTo(Panels, BuildDetail(), FMargin(0.f), VAlign_Top);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* Back = Root->AddChildToOverlay(Veil);
	Back->SetHorizontalAlignment(HAlign_Fill);
	Back->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* Front = Root->AddChildToOverlay(Panels);
	Front->SetHorizontalAlignment(HAlign_Center);
	Front->SetVerticalAlignment(VAlign_Center);
	Shown.SetNum(SlotCount);
	return Root;
}

UWidget* UOdysseeInventoryWidget::BuildGrid()
{
	Slots.SetNum(SlotCount);
	UUniformGridPanel* Bag = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),
		TEXT("BagGrid"));
	Bag->SetSlotPadding(FMargin(4.f));
	for (int32 Index = ToolbarCount; Index < SlotCount; ++Index)
	{
		const int32 Cell = Index - ToolbarCount;
		Bag->AddChildToUniformGrid(MakeSlot(Index), Cell / Columns, Cell % Columns);
	}
	UHorizontalBox* Toolbar = MakeRow(TEXT("ToolbarRow"));
	for (int32 Index = 0; Index < ToolbarCount; ++Index)
	{
		AddTo(Toolbar, MakeSlot(Index), FMargin(4.f, 0.f));
	}

	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("Title", "Inventaire"), Column);
	AddTo(Column, Bag, FMargin(0.f, 0.f, 0.f, 14.f), HAlign_Center);
	UTextBlock* ToolbarTitle = MakeText(NAME_None, LOCTEXT("Toolbar", "BARRE D'OUTILS"), 11, true,
		DimColor());
	ToolbarTitle->SetFont(OdysseeUi::Font(11, true, 250));
	AddTo(Column, ToolbarTitle, FMargin(4.f, 0.f, 0.f, 6.f), HAlign_Left);
	AddTo(Column, Toolbar, FMargin(0.f), HAlign_Center);
	return Panel;
}

UOdysseeInventorySlotWidget* UOdysseeInventoryWidget::MakeSlot(int32 Index)
{
	UOdysseeInventorySlotWidget* Cell = MakeChild<UOdysseeInventorySlotWidget>(*FString::Printf(TEXT("Slot%d"), Index));
	Cell->SetSlot(Index);
	if (Index < ToolbarCount)
	{
		Cell->SetKey(Index + 1); // même numéro que dans le HUD
	}
	else
	{
		Cell->KeyText->SetText(FText::GetEmpty());
	}
	Cell->OnSlotDropped.BindUObject(this, &UOdysseeInventoryWidget::ForwardMove);
	Cell->OnSlotHovered.BindUObject(this, &UOdysseeInventoryWidget::ShowDetail);
	Slots[Index] = Cell;
	return Cell;
}

UWidget* UOdysseeInventoryWidget::BuildDetail()
{
	DetailIcon = MakeImage(TEXT("DetailIcon"), TEXT("outil_baton"), FVector2D(DetailIconSize));
	DetailName = MakeText(TEXT("DetailName"), LOCTEXT("NoItem", "Survolez un objet"), 20, true);
	DetailName->SetAutoWrapText(true);
	DetailCount = MakeText(TEXT("DetailCount"), FText::GetEmpty(), 14, false, DimColor());
	UTextBlock* Hint = MakeText(NAME_None, LOCTEXT("Hint", "Glissez un objet sur une autre case pour le "
		"déplacer, ou sur le même objet pour regrouper."), 12, false, DimColor());
	Hint->SetAutoWrapText(true);

	UVerticalBox* Column = MakeColumn();
	AddTo(Column, MakeSized(DetailIcon, FVector2D(DetailIconSize)), FMargin(0.f, 6.f, 0.f, 14.f), HAlign_Center);
	AddTo(Column, DetailName, FMargin(0.f, 0.f, 0.f, 4.f), HAlign_Left);
	AddTo(Column, DetailCount, FMargin(0.f, 0.f, 0.f, 18.f), HAlign_Left);
	AddTo(Column, Hint, FMargin(0.f), HAlign_Left);
	DetailIcon->SetVisibility(ESlateVisibility::Hidden);
	return MakePanel(TEXT("DetailPanel"), MakeSized(Column, FVector2D(DetailWidth, 360.f)), FMargin(24.f));
}

void UOdysseeInventoryWidget::ShowSlots(const TArray<FOdysseeSlotView>& Views)
{
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		const FOdysseeSlotView View = Views.IsValidIndex(Index) ? Views[Index] : FOdysseeSlotView();
		if (Shown[Index] == View)
		{
			continue; // inchangée : rien à redessiner (les icônes rechargeraient une texture)
		}
		Shown[Index] = View;
		const bool bFilled = View.Count > 0;
		Slots[Index]->SetFilled(bFilled);
		Slots[Index]->ShowItem(bFilled && !View.Icon.IsEmpty() ? *View.Icon : nullptr, View.Count, -1.f);
	}
	ShowDetail(Hovered);
}

void UOdysseeInventoryWidget::ShowDetail(int32 Index)
{
	Hovered = Index;
	const bool bFilled = Shown.IsValidIndex(Index) && Shown[Index].Count > 0;
	DetailIcon->SetVisibility(bFilled && !Shown[Index].Icon.IsEmpty() ? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Hidden);
	if (!bFilled)
	{
		DetailName->SetText(Index < 0 ? LOCTEXT("NoItem", "Survolez un objet") : LOCTEXT("EmptySlot", "Case vide"));
		DetailCount->SetText(FText::GetEmpty());
		return;
	}
	const FOdysseeSlotView& View = Shown[Index];
	if (!View.Icon.IsEmpty())
	{
		DetailIcon->SetBrush(OdysseeUi::ImageBrush(*View.Icon, FVector2D(DetailIconSize), Theme));
		DetailIcon->SetColorAndOpacity(IconTint());
	}
	DetailName->SetText(View.Name);
	DetailCount->SetText(View.Count > 1 ? FText::Format(LOCTEXT("Count", "Quantité : {0}"), View.Count)
		: FText::GetEmpty());
}

void UOdysseeInventoryWidget::ForwardMove(int32 From, int32 To)
{
	OnMoveRequested.ExecuteIfBound(From, To);
}

#undef LOCTEXT_NAMESPACE
