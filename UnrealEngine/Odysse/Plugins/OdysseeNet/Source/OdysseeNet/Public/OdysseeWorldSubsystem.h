// La carte vue par la simulation, côté client : le même fichier monde que le serveur (T041).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "odyssee/sim/Terrain.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include "OdysseeWorldSubsystem.generated.h"

/**
 * Charge une fois, au lancement du jeu, le fichier monde `odyssee.world` : sol, murs et point
 * d'apparition, exactement ceux du serveur. Le mouvement du joueur (UOdysseeMovementComponent)
 * s'en sert à la place de la physique d'Unreal (ADR 0003, règle 1).
 *
 * Fichier cherché, dans l'ordre :
 *  1. `<Projet>/World/odyssee.world` (jeu empaqueté : à copier à côté du projet) ;
 *  2. `tools/worldgen/output/odyssee.world` du dépôt (développement, après `git lfs pull`).
 * Sans fichier : sol plat provisoire, à la hauteur de celui du serveur lancé sans carte.
 */
UCLASS()
class ODYSSEENET_API UOdysseeWorldSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Le sol et les murs de la simulation (la carte, ou le sol plat). */
	const odyssee::sim::Terrain& GetTerrain() const;

	/** La carte chargée, ou nullptr (sol plat). */
	const odyssee::world::WorldTerrain* GetMap() const { return Map.Get(); }

	/** Vrai si la vraie carte a été chargée. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Monde")
	bool HasMap() const { return Map.IsValid(); }

	/** Fichier chargé (vide sans carte). */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Monde")
	FString GetMapPath() const { return MapPath; }

private:
	static FString FindMapFile();
	void LoadMap(const FString& Path);

	TUniquePtr<odyssee::world::WorldTerrain> Map;
	TUniquePtr<odyssee::sim::FlatTerrain> Flat;
	FString MapPath;
};
