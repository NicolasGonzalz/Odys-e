#include "OdysseeResourceActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Taille (en m : les formes de base mesurent 1 m) et teinte d'une forme provisoire. */
	struct FShapeLook
	{
		FVector Size;
		FLinearColor Color;
	};

	/** Mêmes proportions et couleurs que le client de debug (client/src/ResourceShapes.cpp). */
	FShapeLook LookOf(EOdysseeResourceShape Shape)
	{
		switch (Shape)
		{
		case EOdysseeResourceShape::Tree:
			return {FVector(0.65, 0.65, 4.0), FLinearColor(FColor(110, 72, 40))};
		case EOdysseeResourceShape::Rock:
			return {FVector(1.4, 1.2, 1.0), FLinearColor(FColor(130, 130, 130))};
		case EOdysseeResourceShape::Bush:
			return {FVector(1.2, 1.2, 1.0), FLinearColor(FColor(70, 140, 60))};
		case EOdysseeResourceShape::Vein:
			return {FVector(1.2, 1.2, 0.6), FLinearColor(FColor(90, 60, 50))};
		case EOdysseeResourceShape::Unknown:
			break;
		}
		return {FVector(0.8, 0.8, 0.8), FLinearColor(FColor(200, 122, 255))};
	}

	/** Une ressource épuisée : plus petite… */
	constexpr double DepletedScale = 0.6;

	/** … et délavée vers le blanc. */
	FLinearColor Faded(const FLinearColor& Color)
	{
		const FLinearColor Pale(FColor(235, 235, 235));
		return Color + (Pale - Color) * (2.0f / 3.0f);
	}
}

AOdysseeResourceActor::AOdysseeResourceActor()
{
	PrimaryActorTick.bCanEverTick = false; // mise à jour par UOdysseeNetSubsystem

	Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Anchor"));
	RootComponent = Anchor;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Anchor);
	Body->SetCollisionProfileName(TEXT("NoCollision")); // collisions de référence : celles du serveur
	Body->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	TreeMesh = Cylinder.Object;
	RockMesh = Cube.Object;
	BushMesh = Sphere.Object;
	VeinMesh = Cube.Object;
	TintMaterial = Material.Object;
}

void AOdysseeResourceActor::ShowAt(const FVector& BaseLocation)
{
	if (!GetActorLocation().Equals(BaseLocation))
	{
		SetActorLocation(BaseLocation);
	}
}

void AOdysseeResourceActor::ShowState(FName NewKey, EOdysseeResourceShape NewShape, bool bNewDepleted)
{
	const bool bSame = NewKey == ResourceKey && NewShape == Shape && bNewDepleted == bDepleted;
	if (bShown && bSame)
	{
		return;
	}
	bShown = true;
	ResourceKey = NewKey;
	Shape = NewShape;
	bDepleted = bNewDepleted;

	if (bUseProvisionalShapes)
	{
		UpdateLook();
	}
	OnResourceStateChanged();
}

void AOdysseeResourceActor::UpdateLook()
{
	const FShapeLook Look = LookOf(Shape);
	const FVector Size = Look.Size * (bDepleted ? DepletedScale : 1.0);

	// Les formes de base sont centrées : on les monte de leur demi-hauteur pour poser la base au sol.
	Body->SetStaticMesh(MeshFor(Shape));
	Body->SetRelativeScale3D(Size);
	Body->SetRelativeLocation(FVector(0.0, 0.0, Size.Z * 50.0));
	SetTint(bDepleted ? Faded(Look.Color) : Look.Color);
}

UStaticMesh* AOdysseeResourceActor::MeshFor(EOdysseeResourceShape ForShape) const
{
	switch (ForShape)
	{
	case EOdysseeResourceShape::Tree:
		return TreeMesh;
	case EOdysseeResourceShape::Bush:
		return BushMesh;
	case EOdysseeResourceShape::Vein:
		return VeinMesh;
	case EOdysseeResourceShape::Rock:
	case EOdysseeResourceShape::Unknown:
		break;
	}
	return RockMesh;
}

void AOdysseeResourceActor::SetTint(const FLinearColor& Color)
{
	if (!Tint && TintMaterial)
	{
		Tint = UMaterialInstanceDynamic::Create(TintMaterial, this);
		Body->SetMaterial(0, Tint);
	}
	if (Tint)
	{
		Tint->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
