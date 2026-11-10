#include "World/OdysseeScatterActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AOdysseeScatterActor::AOdysseeScatterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Instances =
		CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Instances"));
	Instances->SetMobility(EComponentMobility::Static);
	RootComponent = Instances;
}

void AOdysseeScatterActor::SetInstances(UStaticMesh* Mesh, UMaterialInterface* Material,
                                        const TArray<FTransform>& WorldTransforms)
{
	// Modify() : la modification est enregistrée avec le niveau (et annulable dans l'éditeur).
	Modify();
	Instances->Modify();

	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);

	if (Material != nullptr)
	{
		for (int32 Slot = 0; Slot < Instances->GetNumMaterials(); ++Slot)
		{
			Instances->SetMaterial(Slot, Material);
		}
	}

	constexpr bool bReturnIndices = false;
	constexpr bool bWorldSpace = true;
	Instances->AddInstances(WorldTransforms, bReturnIndices, bWorldSpace);
}

void AOdysseeScatterActor::SetInstancesCollision(bool bEnabled)
{
	Instances->Modify();
	Instances->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics
	                                        : ECollisionEnabled::NoCollision);
}

void AOdysseeScatterActor::SetInstancesCullDistance(float EndDistanceCm)
{
	Instances->Modify();
	Instances->SetCullDistances(0, FMath::RoundToInt(EndDistanceCm));
}

int32 AOdysseeScatterActor::GetInstanceCount() const
{
	return Instances->GetInstanceCount();
}
