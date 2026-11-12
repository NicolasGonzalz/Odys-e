// Jauge ronde low-poly (faim, soif) : un anneau à facettes qui se remplit, une icône au centre.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeGaugeWidget.generated.h"

class UImage;

UCLASS()
class ODYSSE_API UOdysseeGaugeWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Icône (nom de texture d'interface, ex. « icone_faim ») et couleur de la jauge. */
	void Configure(const TCHAR* InIcon, const FLinearColor& InColor);

	/** Remplissage, de 0 à 1. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|UI")
	void SetProgress(float InProgress);

protected:
	virtual UWidget* BuildTree() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void PaintSegment(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry,
		const FVector2D& From, const FVector2D& To, const FLinearColor& Tint) const;

	UPROPERTY()
	TObjectPtr<UImage> Icon;
	FLinearColor Color = FLinearColor::White;
	float Progress = 1.f;
};
