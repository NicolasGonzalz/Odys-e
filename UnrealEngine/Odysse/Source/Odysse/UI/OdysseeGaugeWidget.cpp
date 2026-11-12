#include "UI/OdysseeGaugeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Rendering/DrawElements.h"
#include "UI/OdysseeUiStyle.h"

namespace
{
	constexpr int32 Sides = 10;           // anneau à 10 facettes : rond, mais low-poly
	constexpr float Thickness = 4.f;
	constexpr float GaugeSize = 58.f;
}

void UOdysseeGaugeWidget::Configure(const TCHAR* InIcon, const FLinearColor& InColor)
{
	Color = InColor;
	Icon->SetBrush(OdysseeUi::ImageBrush(InIcon, FVector2D(26.f)));
	Icon->SetColorAndOpacity(InColor);
}

void UOdysseeGaugeWidget::SetProgress(float InProgress)
{
	Progress = FMath::Clamp(InProgress, 0.f, 1.f);
}

UWidget* UOdysseeGaugeWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* Back = Root->AddChildToOverlay(MakeImage(NAME_None, TEXT("case"), FVector2D(GaugeSize)));
	Back->SetHorizontalAlignment(HAlign_Center);
	Back->SetVerticalAlignment(VAlign_Center);
	Icon = MakeImage(TEXT("Icon"), TEXT("icone_faim"), FVector2D(26.f), Color);
	UOverlaySlot* Center = Root->AddChildToOverlay(Icon);
	Center->SetHorizontalAlignment(HAlign_Center);
	Center->SetVerticalAlignment(VAlign_Center);
	return MakeSized(Root, FVector2D(GaugeSize));
}

int32 UOdysseeGaugeWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
		InWidgetStyle, bParentEnabled) + 1;
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Middle = Size * 0.5;
	const double Radius = FMath::Min(Size.X, Size.Y) * 0.5 - Thickness - 3.0;
	const double Filled = Progress * Sides;
	for (int32 Side = 0; Side < Sides; ++Side)
	{
		// En partant du haut, dans le sens des aiguilles d'une montre.
		const double A0 = FMath::DegreesToRadians(-90.0 + 360.0 * Side / Sides);
		const double A1 = FMath::DegreesToRadians(-90.0 + 360.0 * (Side + 1) / Sides);
		const FVector2D From = Middle + Radius * FVector2D(FMath::Cos(A0), FMath::Sin(A0));
		const FVector2D To = Middle + Radius * FVector2D(FMath::Cos(A1), FMath::Sin(A1));
		PaintSegment(OutDrawElements, Layer, AllottedGeometry, From, To, FLinearColor(0.f, 0.f, 0.f, 0.5f));
		if (Side < Filled)
		{
			const FVector2D End = FMath::Lerp(From, To, FMath::Min(Filled - Side, 1.0));
			PaintSegment(OutDrawElements, Layer + 1, AllottedGeometry, From, End, Color);
		}
	}
	return Layer + 1;
}

void UOdysseeGaugeWidget::PaintSegment(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry,
	const FVector2D& From, const FVector2D& To, const FLinearColor& Tint) const
{
	const TArray<FVector2D> Points = {From, To};
	FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Tint,
		true, Thickness);
}
