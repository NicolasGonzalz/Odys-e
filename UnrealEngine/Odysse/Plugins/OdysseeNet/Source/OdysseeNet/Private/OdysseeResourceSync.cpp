#include "OdysseeResourceSync.h"

#include "OdysseeNetSettings.h"
#include "OdysseeResourceActor.h"
#include "OdysseeUnits.h"

#include "Engine/World.h"

#include <cstdint>

namespace
{
	EOdysseeResourceShape ToShape(odyssee::data::ResourceShape Shape)
	{
		switch (Shape)
		{
		case odyssee::data::ResourceShape::Tree:
			return EOdysseeResourceShape::Tree;
		case odyssee::data::ResourceShape::Rock:
			return EOdysseeResourceShape::Rock;
		case odyssee::data::ResourceShape::Bush:
			return EOdysseeResourceShape::Bush;
		case odyssee::data::ResourceShape::Vein:
			return EOdysseeResourceShape::Vein;
		}
		return EOdysseeResourceShape::Unknown;
	}

	/** Type et état d'une entité ressource (drapeaux : rang du type sur 7 bits, bit épuisée). */
	void ShowEntity(AOdysseeResourceActor& Actor, const odyssee::net::EntityState& Entity,
		const odyssee::data::ResourceCatalog* Types)
	{
		const odyssee::data::ResourceTypeId TypeId{
			static_cast<std::uint8_t>(Entity.flags & odyssee::net::kResourceTypeMask)};
		const odyssee::data::ResourceDef* Def = Types ? Types->type(TypeId) : nullptr;
		const FName Key = Def ? FName(UTF8_TO_TCHAR(Def->key.c_str())) : NAME_None;
		const EOdysseeResourceShape Shape = Def ? ToShape(Def->shape) : EOdysseeResourceShape::Unknown;
		const bool bDepleted = (Entity.flags & odyssee::net::kResourceDepleted) != 0;

		Actor.ShowAt(OdysseeUnits::ToUnreal(Entity.position));
		Actor.ShowState(Key, Shape, bDepleted);
	}
}

void UOdysseeResourceSync::Show(const odyssee::net::Snapshot& Snapshot, UWorld& World,
	const odyssee::data::ResourceCatalog* Types)
{
	TSet<int32> Seen;
	for (const odyssee::net::EntityState& Entity : Snapshot.entities)
	{
		if (Entity.kind != odyssee::net::EntityKind::Resource)
		{
			continue;
		}
		const int32 Id = static_cast<int32>(Entity.id);
		Seen.Add(Id);
		if (AOdysseeResourceActor* Actor = FindOrSpawn(Id, World))
		{
			ShowEntity(*Actor, Entity, Types);
		}
	}
	RemoveUnseen(Seen);
}

void UOdysseeResourceSync::RemoveAll()
{
	RemoveUnseen(TSet<int32>());
}

AOdysseeResourceActor* UOdysseeResourceSync::FindOrSpawn(int32 Id, UWorld& World)
{
	// Après un changement de carte, les acteurs de l'ancien monde ont disparu : on recrée.
	const TObjectPtr<AOdysseeResourceActor>* Found = Actors.Find(Id);
	if (Found && IsValid(*Found) && (*Found)->GetWorld() == &World)
	{
		return Found->Get();
	}
	return Spawn(Id, World);
}

AOdysseeResourceActor* UOdysseeResourceSync::Spawn(int32 Id, UWorld& World)
{
	UClass* Class = GetDefault<UOdysseeNetSettings>()->ResourceClass.LoadSynchronous();
	if (!Class)
	{
		Class = AOdysseeResourceActor::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AOdysseeResourceActor* Actor = World.SpawnActor<AOdysseeResourceActor>(Class, FTransform::Identity, Params);
	if (Actor)
	{
		Actor->EntityId = Id;
		Actors.Add(Id, Actor); // remplace l'éventuel acteur d'un monde disparu
	}
	return Actor;
}

void UOdysseeResourceSync::RemoveUnseen(const TSet<int32>& Seen)
{
	for (auto It = Actors.CreateIterator(); It; ++It)
	{
		if (Seen.Contains(It.Key()))
		{
			continue;
		}
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy(); // épuisée ne veut pas dire partie : sortie de la zone d'intérêt
		}
		It.RemoveCurrent();
	}
}
