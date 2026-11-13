#include "OdysseeMovementComponent.h"

#include "OdysseeUnits.h"
#include "OdysseeWorldSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/Quantize.hpp"

#include <cstdint>

namespace
{
	constexpr float TickSeconds = 1.0f / static_cast<float>(odyssee::kTickRate);
	constexpr int32 MaxStepsPerFrame = 5; ///< après un gel, on ne rattrape pas indéfiniment

	std::uint16_t ButtonIf(bool bDown, odyssee::net::InputButton Button)
	{
		return bDown ? static_cast<std::uint16_t>(Button) : std::uint16_t{0};
	}
}

UOdysseeMovementComponent::UOdysseeMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UOdysseeMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UOdysseeWorldSubsystem* WorldMap =
		GameInstance ? GameInstance->GetSubsystem<UOdysseeWorldSubsystem>() : nullptr;
	Terrain = WorldMap ? &WorldMap->GetTerrain() : nullptr;

	const FVector Feet = GetOwner()->GetActorLocation() - FVector(0.0, 0.0, FeetToActorOffset);
	TeleportFeetTo(Feet);
}

void UOdysseeMovementComponent::AddMoveInput(float Forward, float Right)
{
	PendingMove += FVector2D(Forward, Right);
}

void UOdysseeMovementComponent::TeleportFeetTo(FVector Feet)
{
	State.position = OdysseeUnits::FromUnreal(Feet);
	State.velocity = {0, 0, 0};
	Previous = State;
	Accumulated = 0.0f;
	ShowBetweenSteps(1.0f);
}

void UOdysseeMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Terrain == nullptr)
	{
		return;
	}

	// Le déplacement arrive à chaque image tant que la touche est tenue : on garde le dernier pour
	// le prochain pas de simulation, puis on repart de zéro (touche relâchée = plus d'entrée).
	MoveOfStep = PendingMove;
	PendingMove = FVector2D::ZeroVector;

	Accumulated += DeltaTime;
	for (int32 Step = 0; Step < MaxStepsPerFrame && Accumulated >= TickSeconds; ++Step)
	{
		StepSimulation();
		Accumulated -= TickSeconds;
	}
	if (Accumulated >= TickSeconds)
	{
		Accumulated = 0.0f; // trop de retard : on repart du présent
	}
	ShowBetweenSteps(Accumulated / TickSeconds);
}

odyssee::net::InputCommand UOdysseeMovementComponent::BuildCommand() const
{
	odyssee::net::InputCommand Command;
	// FVector2D et FRotator sont en double dans UE5 : conversions explicites vers les float de
	// shared/.
	Command.moveForward =
		odyssee::net::quantizeAxis(FMath::Clamp(static_cast<float>(MoveOfStep.X), -1.0f, 1.0f));
	Command.moveRight =
		odyssee::net::quantizeAxis(FMath::Clamp(static_cast<float>(MoveOfStep.Y), -1.0f, 1.0f));

	const APawn* Pawn = Cast<APawn>(GetOwner());
	const FRotator Look = Pawn ? Pawn->GetControlRotation() : GetOwner()->GetActorRotation();
	Command.yaw = OdysseeUnits::YawFromDegrees(static_cast<float>(Look.Yaw));
	Command.pitch = OdysseeUnits::PitchFromDegrees(static_cast<float>(Look.Pitch));

	Command.buttons = static_cast<std::uint16_t>(
		ButtonIf(bJumpHeld, odyssee::net::InputButton::Jump) |
		ButtonIf(bSprintHeld, odyssee::net::InputButton::Sprint) |
		ButtonIf(bCrouchHeld, odyssee::net::InputButton::Crouch));
	return Command;
}

void UOdysseeMovementComponent::StepSimulation()
{
	Previous = State;
	State = odyssee::sim::step(State, BuildCommand(), *Terrain);
}

void UOdysseeMovementComponent::ShowBetweenSteps(float Alpha)
{
	const FVector Feet = FMath::Lerp(ToFeetLocation(Previous), ToFeetLocation(State), Alpha);
	GetOwner()->SetActorLocation(Feet + FVector(0.0, 0.0, FeetToActorOffset), false, nullptr,
		ETeleportType::TeleportPhysics);
}

FVector UOdysseeMovementComponent::ToFeetLocation(const odyssee::sim::PlayerState& Of) const
{
	return OdysseeUnits::ToUnreal(Of.position);
}

FVector UOdysseeMovementComponent::GetVelocity() const
{
	// Vitesse de simulation : 1/256 m par tick → cm/s.
	const double Scale = OdysseeUnits::CentimetersPerUnit * odyssee::kTickRate;
	return FVector(State.velocity[0] * Scale, State.velocity[1] * Scale, State.velocity[2] * Scale);
}

bool UOdysseeMovementComponent::IsGrounded() const
{
	return State.has(odyssee::sim::kGrounded);
}

bool UOdysseeMovementComponent::IsCrouched() const
{
	return State.has(odyssee::sim::kCrouched);
}

bool UOdysseeMovementComponent::IsSprinting() const
{
	return State.has(odyssee::sim::kSprinting);
}

float UOdysseeMovementComponent::GetStamina() const
{
	return static_cast<float>(State.stamina) /
		static_cast<float>(odyssee::sim::kDefaultTuning.maxStamina);
}

bool UOdysseeMovementComponent::IsExhausted() const
{
	return State.has(odyssee::sim::kExhausted);
}
