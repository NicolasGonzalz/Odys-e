// Trous dans le Landscape (entrées de grotte), posés par le script de blockout
// (tools/worldgen/unreal/build_blockout.py).

#pragma once

#include "CoreMinimal.h"
#include "LandscapeBlueprintBrushBase.h"
#include "OdysseeHoleBrush.generated.h"

class ALandscape;
class UTextureRenderTarget2D;

/** Un trou rectangulaire, en coordonnées monde (cm), tourné de YawDegrees autour de son centre. */
USTRUCT(BlueprintType)
struct FOdysseeHole
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odyssee")
	FVector2D Center = FVector2D::ZeroVector;

	/** Longueur (le long de l'axe tourné) et largeur, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odyssee")
	FVector2D Size = FVector2D(100.0, 100.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odyssee")
	float YawDegrees = 0.0f;
};

/**
 * Pinceau de Landscape qui perce des trous (couche de visibilité : ni rendu ni collision).
 *
 * Avec les couches d'édition (obligatoires en 5.6), la couche de visibilité ne peut plus être
 * importée par script : un pinceau est la façon prévue de l'écrire. Ses trous font partie du calcul
 * du Landscape, ils suivent donc les grottes quand le script les déplace.
 */
UCLASS()
class ODYSSE_API AOdysseeHoleBrush : public ALandscapeBlueprintBrushBase
{
	GENERATED_BODY()

public:
	AOdysseeHoleBrush(const FObjectInitializer& ObjectInitializer);

	/** Remplace la liste des trous et redemande le calcul du Landscape. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Generation")
	void SetHoles(const TArray<FOdysseeHole>& InHoles);

	/** Rattache le pinceau à la première couche d'édition du Landscape (éditeur seulement). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Generation")
	void AttachToLandscape(ALandscape* Landscape);

	virtual UTextureRenderTarget2D* RenderLayer_Native(const FLandscapeBrushParameters& InParameters) override;

private:
	void DrawHoles(const FLandscapeBrushParameters& InParameters);

	UPROPERTY(EditAnywhere, Category = "Odyssee")
	TArray<FOdysseeHole> Holes;

	/** Copie de la couche reçue, sur laquelle on dessine les trous (réutilisée d'un calcul à l'autre). */
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> Output;
};
