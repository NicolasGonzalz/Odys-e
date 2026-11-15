// Un sac laissé au sol à la mort d'un joueur (T052), tel que le serveur le décrit.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OdysseeBagActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * Un sac du monde, placé par UOdysseeNetSubsystem d'après le dernier snapshot : il est fixe, donc
 * pas d'interpolation. Il se ramasse en passant dessus (c'est le serveur qui décide). L'origine de
 * l'acteur est à sa **base** (au sol).
 *
 * Modèle : le sac à dos (art/exports/objets/sac_a_dos.fbx, importé dans /Game/Odyssee/Objets) ; un
 * bloc brun s'il n'est pas là. Pour un autre rendu : un Blueprint enfant, choisi dans Paramètres du
 * projet → Odyssee → Réseau (Bag Class).
 */
UCLASS(Blueprintable)
class ODYSSEENET_API AOdysseeBagActor : public AActor
{
	GENERATED_BODY()

public:
	AOdysseeBagActor();

	/** Numéro d'entité donné par le serveur. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Sac")
	int32 EntityId = 0;

	/** Place la base du sac (cm). */
	void ShowAt(const FVector& BaseLocation);

protected:
	/** Choisit le modèle (aussi dans l'éditeur : aperçu des Blueprints). */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Modèle du sac ; s'il manque, un bloc provisoire. */
	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Sac")
	TSoftObjectPtr<UStaticMesh> BagMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Anchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

private:
	void ShowProvisionalShape();

	UPROPERTY()
	TObjectPtr<UStaticMesh> ProvisionalMesh;
};
