#include "UI/OdysseeInventorySlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"

void UOdysseeInventorySlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::Visible); // la case reçoit la souris (survol, glisser, lâcher)
}

FReply UOdysseeInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (bFilled && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(Event, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}

void UOdysseeInventorySlotWidget::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event,
	UDragDropOperation*& OutOperation)
{
	UOdysseeSlotDragOperation* Drag = NewObject<UOdysseeSlotDragOperation>(this);
	Drag->FromSlot = SlotIndex;
	// L'icône suit la souris ; la case d'origine reste en place jusqu'à la réponse du serveur.
	UImage* Visual = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Visual->SetBrush(Icon->GetBrush());
	Visual->SetDesiredSizeOverride(FVector2D(72.f));
	Visual->SetRenderOpacity(0.85f);
	Drag->DefaultDragVisual = Visual;
	Drag->Pivot = EDragPivot::CenterCenter;
	OutOperation = Drag;
}

bool UOdysseeInventorySlotWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event,
	UDragDropOperation* Operation)
{
	SetActive(false);
	const UOdysseeSlotDragOperation* Drag = Cast<UOdysseeSlotDragOperation>(Operation);
	if (!Drag || Drag->FromSlot == SlotIndex)
	{
		return false;
	}
	OnSlotDropped.ExecuteIfBound(Drag->FromSlot, SlotIndex); // échange ou regroupement : le serveur décide
	return true;
}

void UOdysseeInventorySlotWidget::NativeOnDragEnter(const FGeometry& Geometry, const FDragDropEvent& Event,
	UDragDropOperation* Operation)
{
	SetActive(Cast<UOdysseeSlotDragOperation>(Operation) != nullptr); // la case visée s'allume
}

void UOdysseeInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	SetActive(false);
}

void UOdysseeInventorySlotWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	OnSlotHovered.ExecuteIfBound(SlotIndex);
}
