// Une case de l'écran d'inventaire : la case de la barre d'outils, plus le glisser-déposer.

#pragma once

#include "Blueprint/DragDropOperation.h"
#include "CoreMinimal.h"
#include "UI/OdysseeToolSlotWidget.h"
#include "OdysseeInventorySlotWidget.generated.h"

/** Case lâchée sur une autre : (case de départ, case d'arrivée), numéros du serveur (0 à 24). */
DECLARE_DELEGATE_TwoParams(FOdysseeSlotDropped, int32, int32);
DECLARE_DELEGATE_OneParam(FOdysseeSlotHovered, int32);

/** Ce qu'on transporte en glissant : le numéro de la case de départ. */
UCLASS()
class ODYSSE_API UOdysseeSlotDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	int32 FromSlot = -1;
};

UCLASS()
class ODYSSE_API UOdysseeInventorySlotWidget : public UOdysseeToolSlotWidget
{
	GENERATED_BODY()

public:
	/** Numéro de la case côté serveur, et si elle contient un objet (sinon, rien à attraper). */
	void SetSlot(int32 InSlot) { SlotIndex = InSlot; }
	void SetFilled(bool bInFilled) { bFilled = bInFilled; }

	FOdysseeSlotDropped OnSlotDropped;
	FOdysseeSlotHovered OnSlotHovered;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event,
		UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event,
		UDragDropOperation* Operation) override;
	virtual void NativeOnDragEnter(const FGeometry& Geometry, const FDragDropEvent& Event,
		UDragDropOperation* Operation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& Event, UDragDropOperation* Operation) override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
	int32 SlotIndex = 0;
	bool bFilled = false;
};
