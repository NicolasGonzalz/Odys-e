#include "OdysseeBagActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Bloc provisoire : un sac de 50 × 35 × 60 cm (la forme de base mesure 1 m, centrée). */
	const FVector ProvisionalSize(0.5, 0.35, 0.6);
}

AOdysseeBagActor::AOdysseeBagActor()
{
	PrimaryActorTick.bCanEverTick = false; // placé par UOdysseeNetSubsystem

	Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Anchor"));
	RootComponent = Anchor;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Anchor);
	Body->SetCollisionProfileName(TEXT("NoCollision")); // le ramassage est décidé par le serveur
	Body->SetCanEverAffectNavigation(false);

	BagMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Odyssee/Objets/SM_SacADos.SM_SacADos")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ProvisionalMesh = Cube.Object;
}

void AOdysseeBagActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (UStaticMesh* Mesh = BagMesh.LoadSynchronous())
	{
		Body->SetStaticMesh(Mesh);
		return;
	}
	ShowProvisionalShape();
}

void AOdysseeBagActor::ShowProvisionalShape()
{
	Body->SetStaticMesh(ProvisionalMesh);
	Body->SetRelativeScale3D(ProvisionalSize);
	Body->SetRelativeLocation(FVector(0.0, 0.0, ProvisionalSize.Z * 50.0)); // posé sur sa base
	Body->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(FLinearColor(FColor(110, 80, 50))));
}

void AOdysseeBagActor::ShowAt(const FVector& BaseLocation)
{
	if (!GetActorLocation().Equals(BaseLocation))
	{
		SetActorLocation(BaseLocation);
	}
}
