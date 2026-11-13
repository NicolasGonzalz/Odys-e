#include "UI/OdysseeToolSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/OdysseeUiStyle.h"

namespace
{
	constexpr float SlotSize = 104.f;
	constexpr float IconSize = 80.f;

	void Place(UOverlaySlot* Cell, EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Padding)
	{
		Cell->SetHorizontalAlignment(H);
		Cell->SetVerticalAlignment(V);
		Cell->SetPadding(Padding);
	}
}

UWidget* UOdysseeToolSlotWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Frame = MakeImage(TEXT("Frame"), TEXT("case"), FVector2D(SlotSize));
	Icon = MakeImage(TEXT("Icon"), TEXT("outil_baton"), FVector2D(IconSize));
	KeyText = MakeText(TEXT("KeyText"), FText::AsNumber(1), 15, true, OdysseeUi::BoneDim);
	CountText = MakeText(TEXT("CountText"), FText::GetEmpty(), 17, true);
	DurabilityBar = MakeBar(TEXT("DurabilityBar"), FLinearColor(0.55f, 0.80f, 0.40f), 1.f);
	Place(Root->AddChildToOverlay(Frame), HAlign_Fill, VAlign_Fill, FMargin(0.f));
	Place(Root->AddChildToOverlay(Icon), HAlign_Center, VAlign_Center, FMargin(0.f, 0.f, 0.f, 4.f));
	Place(Root->AddChildToOverlay(KeyText), HAlign_Left, VAlign_Top, FMargin(11.f, 6.f));
	Place(Root->AddChildToOverlay(CountText), HAlign_Right, VAlign_Bottom, FMargin(11.f, 0.f, 12.f, 14.f));
	Place(Root->AddChildToOverlay(MakeSized(DurabilityBar, FVector2D(SlotSize - 30.f, 7.f))), HAlign_Center,
		VAlign_Bottom, FMargin(0.f, 0.f, 0.f, 8.f));
	USizeBox* Box = MakeSized(Root, FVector2D(SlotSize));
	ShowItem(nullptr, 0, -1.f); // case vide tant que le jeu n'y met rien
	return Box;
}

void UOdysseeToolSlotWidget::SetKey(int32 InKey)
{
	KeyText->SetText(FText::AsNumber(InKey));
}

void UOdysseeToolSlotWidget::ShowItem(const TCHAR* InIcon, int32 Count, float Durability)
{
	const bool bEmpty = InIcon == nullptr || *InIcon == TEXT('\0');
	Icon->SetVisibility(bEmpty ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	if (!bEmpty)
	{
		Icon->SetBrush(OdysseeUi::ImageBrush(InIcon, FVector2D(IconSize)));
	}
	CountText->SetText(Count > 1 ? FText::AsNumber(Count) : FText::GetEmpty());
	DurabilityBar->GetParent()->SetVisibility(
		Durability < 0.f || bEmpty ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	DurabilityBar->SetPercent(FMath::Clamp(Durability, 0.f, 1.f));
	// Usure : vert quand l'outil est neuf, rouille quand il va casser.
	DurabilityBar->SetFillColorAndOpacity(
		FLinearColor::LerpUsingHSV(OdysseeUi::Health, FLinearColor(0.55f, 0.80f, 0.40f), Durability));
}

void UOdysseeToolSlotWidget::SetActive(bool bActive)
{
	Frame->SetBrush(OdysseeUi::ImageBrush(bActive ? TEXT("case_active") : TEXT("case"), FVector2D(SlotSize)));
	KeyText->SetColorAndOpacity(FSlateColor(bActive ? OdysseeUi::Rust : OdysseeUi::BoneDim));
}
