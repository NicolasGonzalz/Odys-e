// Groupe d'instances d'un même modèle (arbres, buissons, rochers…), posé par le script de
// génération du monde (tools/worldgen/unreal/build_map.py).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OdysseeScatterActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Un seul modèle, affiché des milliers de fois en un seul appel au GPU (instanciation).
 * Le script crée un acteur par type d'élément et par carré de terrain : World Partition ne charge
 * ainsi que la végétation proche du joueur.
 */
UCLASS()
class ODYSSE_API AOdysseeScatterActor : public AActor
{
	GENERATED_BODY()

public:
	AOdysseeScatterActor();

	/** Remplace le contenu : modèle, matériau (optionnel) et positions en coordonnées monde. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Generation")
	void SetInstances(UStaticMesh* Mesh, UMaterialInterface* Material,
	                  const TArray<FTransform>& WorldTransforms);

	/** Collision pour les arbres et rochers ; aucune pour l'herbe et les fleurs. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Generation")
	void SetInstancesCollision(bool bEnabled);

	/** Distance (en cm) au-delà de laquelle les instances ne sont plus dessinées ; 0 = jamais. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Generation")
	void SetInstancesCullDistance(float EndDistanceCm);

	/** Nombre d'instances (vérification depuis le script). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Generation")
	int32 GetInstanceCount() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Odyssee")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;
};
