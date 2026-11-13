#include "OdysseeMovementComponent.h"

#include "OdysseeNetSubsystem.h"
#include "OdysseeUnits.h"
#include "OdysseeWorldSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
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
	PreviousShown = State.position;
	CurrentShown = State.position;
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
	PreviousShown = CurrentShown;
	UOdysseeNetSubsystem* Net = FindNet();
	if (Net && Net->IsConnected())
	{
		StepOnline(*Net);
	}
	else
	{
		Prediction.Reset();
		StepOffline();
	}
}

void UOdysseeMovementComponent::StepOffline()
{
	State = odyssee::sim::step(State, BuildCommand(), *Terrain);
	CurrentShown = State.position;
}

void UOdysseeMovementComponent::StepOnline(UOdysseeNetSubsystem& Net)
{
	if (!Prediction || PredictionSession != Net.GetSessionId())
	{
		Prediction = MakeUnique<odyssee::client::Prediction>(*Terrain); // nouvelle partie
		PredictionSession = Net.GetSessionId();
		SeenSnapshot = 0;
	}
	FollowServer(Net);
	if (!Prediction->started())
	{
		return; // on attend le premier snapshot : le serveur dit où on apparaît
	}
	if (Prediction->advance(BuildCommand()))
	{
		Net.SendInputs(Prediction->history().recentBatch());
	}
	State = *Prediction->state();
	CurrentShown = Prediction->displayPosition(); // les corrections fondent en quelques pas
}

void UOdysseeMovementComponent::FollowServer(UOdysseeNetSubsystem& Net)
{
	const auto Self = Net.GetPlayerId();
	const auto& Snapshot = Net.GetLatestSnapshot();
	if (!Self || !Snapshot || Net.GetSnapshotVersion() == SeenSnapshot)
	{
		return;
	}
	SeenSnapshot = Net.GetSnapshotVersion();
	if (Prediction->started())
	{
		Prediction->reconcile(*Snapshot, *Self); // T032 : recalage sur la vérité du serveur
	}
	else
	{
		StartPrediction(*Snapshot, *Self);
	}
}

void UOdysseeMovementComponent::StartPrediction(const odyssee::net::Snapshot& Snapshot,
	odyssee::net::PlayerId Self)
{
	if (!Prediction->start(Snapshot, Self))
	{
		return;
	}
	State = *Prediction->state();
	PreviousShown = State.position;
	CurrentShown = State.position;

	// Le serveur donne aussi le cap d'apparition : la caméra le prend.
	if (APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->GetController())
	{
		Pawn->GetController()->SetControlRotation(
			FRotator(0.0, OdysseeUnits::YawToDegrees(State.yaw), 0.0));
	}
}

void UOdysseeMovementComponent::ShowBetweenSteps(float Alpha)
{
	const FVector Feet = FMath::Lerp(OdysseeUnits::ToUnreal(PreviousShown),
		OdysseeUnits::ToUnreal(CurrentShown), static_cast<double>(Alpha));
	GetOwner()->SetActorLocation(Feet + FVector(0.0, 0.0, FeetToActorOffset), false, nullptr,
		ETeleportType::TeleportPhysics);
}

UOdysseeNetSubsystem* UOdysseeMovementComponent::FindNet() const
{
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UOdysseeNetSubsystem>() : nullptr;
}

int32 UOdysseeMovementComponent::GetCorrections() const
{
	return Prediction ? static_cast<int32>(Prediction->corrections()) : 0;
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
