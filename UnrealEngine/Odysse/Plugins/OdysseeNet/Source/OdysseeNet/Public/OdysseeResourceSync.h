// Les ressources récoltables du dernier snapshot, montrées dans le monde (T044).
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/GameMessages.hpp"

#include "OdysseeResourceSync.generated.h"

class AOdysseeResourceActor;
class UWorld;

/**
 * Un AOdysseeResourceActor par entité `Resource` du dernier snapshot : créé quand elle entre dans
 * la zone d'intérêt, mis à jour (type, épuisée) puis détruit quand elle en sort. Les ressources
 * sont fixes : pas d'interpolation, le dernier état connu suffit. Tenu par UOdysseeNetSubsystem.
 */
UCLASS()
class ODYSSEENET_API UOdysseeResourceSync : public UObject
{
	GENERATED_BODY()

public:
	/** Montre les ressources de `Snapshot` dans `World`. `Types` : data/resources.txt (nul : formes
	 *  inconnues). */
	void Show(const odyssee::net::Snapshot& Snapshot, UWorld& World,
		const odyssee::data::ResourceCatalog* Types);

	/** Retire toutes les ressources (déconnexion). */
	void RemoveAll();

private:
	AOdysseeResourceActor* FindOrSpawn(int32 Id, UWorld& World);
	AOdysseeResourceActor* Spawn(int32 Id, UWorld& World);
	void RemoveUnseen(const TSet<int32>& Seen);

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AOdysseeResourceActor>> Actors;
};
