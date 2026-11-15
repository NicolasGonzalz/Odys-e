#include "OdysseeBagSync.h"

#include "OdysseeBagActor.h"
#include "OdysseeNetSettings.h"
#include "OdysseeUnits.h"

#include "Engine/World.h"

void UOdysseeBagSync::Show(const odyssee::net::Snapshot& Snapshot, UWorld& World)
{
	TSet<int32> Seen;
	for (const odyssee::net::EntityState& Entity : Snapshot.entities)
	{
		if (Entity.kind != odyssee::net::EntityKind::Bag)
		{
			continue;
		}
		const int32 Id = static_cast<int32>(Entity.id);
		Seen.Add(Id);
		if (AOdysseeBagActor* Actor = FindOrSpawn(Id, World))
		{
			Actor->ShowAt(OdysseeUnits::ToUnreal(Entity.position));
		}
	}
	RemoveUnseen(Seen);
}

void UOdysseeBagSync::RemoveAll()
{
	RemoveUnseen(TSet<int32>());
}

AOdysseeBagActor* UOdysseeBagSync::FindOrSpawn(int32 Id, UWorld& World)
{
	// Après un changement de carte, les acteurs de l'ancien monde ont disparu : on recrée.
	const TObjectPtr<AOdysseeBagActor>* Found = Actors.Find(Id);
	if (Found && IsValid(*Found) && (*Found)->GetWorld() == &World)
	{
		return Found->Get();
	}
	return Spawn(Id, World);
}

AOdysseeBagActor* UOdysseeBagSync::Spawn(int32 Id, UWorld& World)
{
	UClass* Class = GetDefault<UOdysseeNetSettings>()->BagClass.LoadSynchronous();
	if (!Class)
	{
		Class = AOdysseeBagActor::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AOdysseeBagActor* Actor = World.SpawnActor<AOdysseeBagActor>(Class, FTransform::Identity, Params);
	if (Actor)
	{
		Actor->EntityId = Id;
		Actors.Add(Id, Actor); // remplace l'éventuel acteur d'un monde disparu
	}
	return Actor;
}

void UOdysseeBagSync::RemoveUnseen(const TSet<int32>& Seen)
{
	for (auto It = Actors.CreateIterator(); It; ++It)
	{
		if (Seen.Contains(It.Key()))
		{
			continue;
		}
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy(); // ramassé, ou sorti de la zone d'intérêt
		}
		It.RemoveCurrent();
	}
}
