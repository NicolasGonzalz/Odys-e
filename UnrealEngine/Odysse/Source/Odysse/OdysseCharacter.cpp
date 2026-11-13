// Copyright Epic Games, Inc. All Rights Reserved.

#include "OdysseCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "OdysseeMovementComponent.h"
#include "Odysse.h"

AOdysseCharacter::AOdysseCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Le vrai mouvement : la simulation partagée avec le serveur (T016).
	SimMovement = CreateDefaultSubobject<UOdysseeMovementComponent>(TEXT("Sim Movement"));
	SimMovement->FeetToActorOffset = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
}

void AOdysseCharacter::BeginPlay()
{
	// Avant Super::BeginPlay (qui démarre les composants) : la capsule ne doit plus être poussée
	// ni par la physique ni par le CharacterMovementComponent, seulement par SimMovement.
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Super::BeginPlay();
}

void AOdysseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AOdysseCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AOdysseCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOdysseCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOdysseCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AOdysseCharacter::LookInput);

		// Courir et s'accroupir (actions facultatives, tenues)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AOdysseCharacter::SprintStart);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AOdysseCharacter::SprintEnd);
		}
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AOdysseCharacter::CrouchStart);
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AOdysseCharacter::CrouchEnd);
		}
	}
	else
	{
		UE_LOG(LogOdysse, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AOdysseCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AOdysseCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AOdysseCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AOdysseCharacter::DoMove(float Right, float Forward)
{
	// Menu ouvert : le contrôleur ignore les déplacements (AOdyssePlayerController::SetMenuInput).
	if (GetController() && !GetController()->IsMoveInputIgnored())
	{
		// La direction (selon le cap de la caméra) est calculée par la simulation.
		SimMovement->AddMoveInput(Forward, Right);
	}
}

void AOdysseCharacter::DoJumpStart()
{
	if (GetController() && !GetController()->IsMoveInputIgnored())
	{
		SimMovement->SetJumpHeld(true);
	}
}

void AOdysseCharacter::DoJumpEnd()
{
	SimMovement->SetJumpHeld(false);
}

void AOdysseCharacter::SprintStart()
{
	SimMovement->SetSprintHeld(true);
}

void AOdysseCharacter::SprintEnd()
{
	SimMovement->SetSprintHeld(false);
}

void AOdysseCharacter::CrouchStart()
{
	SimMovement->SetCrouchHeld(true);
}

void AOdysseCharacter::CrouchEnd()
{
	SimMovement->SetCrouchHeld(false);
}
