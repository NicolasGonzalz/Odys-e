// Un autre joueur, tel que le serveur le décrit : placé à chaque image par UOdysseeNetSubsystem.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OdysseeRemotePlayer.generated.h"

class USkeletalMeshComponent;

/**
 * Corps d'un autre joueur, interpolé entre deux snapshots (T033). L'origine de l'acteur est aux
 * **pieds** ; le maillage (le mannequin en attendant les survivants) se règle dans un Blueprint
 * enfant, choisi dans Paramètres du projet → Odyssee → Réseau.
 */
UCLASS(Blueprintable)
class ODYSSEENET_API AOdysseeRemotePlayer : public AActor
{
	GENERATED_BODY()

public:
	AOdysseeRemotePlayer();

	/** Numéro du joueur (0 à 3). */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Reseau")
	int32 PlayerId = 0;

	/** Vitesse estimée d'après le déplacement affiché (cm/s), pour les animations. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Reseau")
	FVector ShownVelocity = FVector::ZeroVector;

	/** Place le joueur (pieds, cap en degrés) ; appelé à chaque image. */
	void ShowAt(const FVector& NewFeet, float YawDegrees, float DeltaTime);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Feet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Body;
};
