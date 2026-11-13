#include "OdysseeRemotePlayer.h"

#include "Components/SkeletalMeshComponent.h"

AOdysseeRemotePlayer::AOdysseeRemotePlayer()
{
	PrimaryActorTick.bCanEverTick = false; // placé par UOdysseeNetSubsystem

	Feet = CreateDefaultSubobject<USceneComponent>(TEXT("Feet"));
	RootComponent = Feet;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Feet);
	Body->SetCollisionProfileName(TEXT("NoCollision"));
	// Le mannequin d'Unreal regarde vers +Y : un quart de tour pour qu'il regarde vers +X.
	Body->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
}

void AOdysseeRemotePlayer::ShowAt(const FVector& NewFeet, float YawDegrees, float DeltaTime)
{
	if (DeltaTime > 0.0f)
	{
		ShownVelocity = (NewFeet - GetActorLocation()) / DeltaTime;
	}
	SetActorLocationAndRotation(NewFeet, FRotator(0.0, YawDegrees, 0.0));
}
