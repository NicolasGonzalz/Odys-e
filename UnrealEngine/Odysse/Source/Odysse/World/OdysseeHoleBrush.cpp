#include "World/OdysseeHoleBrush.h"

#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Landscape.h"
#include "LandscapeEditTypes.h"

AOdysseeHoleBrush::AOdysseeHoleBrush(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetCanAffectHeightmap(false);
	SetCanAffectWeightmap(false);
	SetCanAffectVisibilityLayer(true);
}

void AOdysseeHoleBrush::SetHoles(const TArray<FOdysseeHole>& InHoles)
{
	Holes = InHoles;
	RequestLandscapeUpdate(/*bInUserTriggered=*/true);
}

void AOdysseeHoleBrush::AttachToLandscape(ALandscape* Landscape)
{
#if WITH_EDITOR
	if (Landscape != nullptr && GetOwningLandscape() != Landscape)
	{
		Landscape->AddBrushToLayer(0, this);
	}
	RequestLandscapeUpdate(/*bInUserTriggered=*/true);
#endif
}

UTextureRenderTarget2D* AOdysseeHoleBrush::RenderLayer_Native(const FLandscapeBrushParameters& InParameters)
{
	UTextureRenderTarget2D* Combined = InParameters.CombinedResult;
	UE_LOG(LogTemp, Verbose, TEXT("[OdysseeHoleBrush] couche %s (%s), zone %dx%d, %d trous"),
		*UEnum::GetValueAsString(InParameters.LayerType), *InParameters.WeightmapLayerName.ToString(),
		InParameters.RenderAreaSize.X, InParameters.RenderAreaSize.Y, Holes.Num());
	if (InParameters.LayerType != ELandscapeToolTargetType::Visibility || Combined == nullptr)
	{
		return Combined; // seule la couche de visibilité nous concerne
	}

	// Une cible à nous, de même taille et même format : on y recopie la couche, puis on perce.
	const bool bNeedsNew = Output == nullptr || Output->SizeX != Combined->SizeX ||
		Output->SizeY != Combined->SizeY || Output->RenderTargetFormat != Combined->RenderTargetFormat;
	if (bNeedsNew)
	{
		Output = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, Combined->SizeX, Combined->SizeY, Combined->RenderTargetFormat, FLinearColor::Black);
	}
	DrawHoles(InParameters);
	return Output;
}

void AOdysseeHoleBrush::DrawHoles(const FLandscapeBrushParameters& InParameters)
{
	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, Output, Canvas, CanvasSize, Context);

	// La couche telle qu'elle est (les autres trous éventuels sont gardés).
	Canvas->K2_DrawTexture(InParameters.CombinedResult, FVector2D::ZeroVector, CanvasSize,
		FVector2D::ZeroVector, FVector2D::UnitVector, FLinearColor::White, BLEND_Opaque);

	// Monde (cm) -> pixels de la zone calculée : la transformation de la zone a l'échelle du
	// Landscape (1 pixel = 1 point du Landscape).
	const FTransform& Area = InParameters.RenderAreaWorldTransform;
	const FVector Scale = Area.GetScale3D();
	for (const FOdysseeHole& Hole : Holes)
	{
		const FVector Center = Area.InverseTransformPosition(FVector(Hole.Center, 0.0));
		const FVector2D SizePx(Hole.Size.X / Scale.X, Hole.Size.Y / Scale.Y);
		const FVector2D TopLeft = FVector2D(Center.X, Center.Y) - SizePx * 0.5;
		Canvas->K2_DrawTexture(nullptr, TopLeft, SizePx, FVector2D::ZeroVector,
			FVector2D::UnitVector, FLinearColor::White, BLEND_Opaque, Hole.YawDegrees,
			FVector2D(0.5, 0.5));
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}
