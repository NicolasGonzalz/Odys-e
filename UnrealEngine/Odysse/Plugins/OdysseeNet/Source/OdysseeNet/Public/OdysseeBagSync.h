// Les sacs au sol du dernier snapshot, montrés dans le monde (T052).
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "odyssee/net/GameMessages.hpp"

#include "OdysseeBagSync.generated.h"

class AOdysseeBagActor;
class UWorld;

/**
 * Un AOdysseeBagActor par entité `Bag` du dernier snapshot : créé quand le sac apparaît (ou entre
 * dans la zone d'intérêt), détruit quand il disparaît (ramassé, ou hors de vue). Les sacs sont
 * fixes : pas d'interpolation. Tenu par UOdysseeNetSubsystem, comme UOdysseeResourceSync.
 */
UCLASS()
class ODYSSEENET_API UOdysseeBagSync : public UObject
{
	GENERATED_BODY()

public:
	/** Montre les sacs de `Snapshot` dans `World`. */
	void Show(const odyssee::net::Snapshot& Snapshot, UWorld& World);

	/** Retire tous les sacs (déconnexion). */
	void RemoveAll();

private:
	AOdysseeBagActor* FindOrSpawn(int32 Id, UWorld& World);
	AOdysseeBagActor* Spawn(int32 Id, UWorld& World);
	void RemoveUnseen(const TSet<int32>& Seen);

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AOdysseeBagActor>> Actors;
};
