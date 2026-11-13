// Mouvement du joueur par la simulation partagée (T016), à la place du CharacterMovementComponent.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "odyssee/client/Prediction.hpp"
#include "odyssee/net/ControlMessages.hpp" // PlayerId
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"

#include <array>
#include <cstdint>

#include "OdysseeMovementComponent.generated.h"

class UOdysseeNetSubsystem;

/**
 * Déplace son acteur avec `odyssee::sim::step`, la même fonction que le serveur, sur le sol et
 * les murs du fichier monde (UOdysseeWorldSubsystem) : pas de physique Unreal (ADR 0003).
 *
 * - La simulation avance à pas fixe, 20 fois par seconde comme le serveur. Entre deux pas,
 *   l'affichage interpole : la caméra reste fluide à 60 images/s et plus.
 * - Les entrées (déplacement, saut, course, accroupi, action principale) sont données par le
 *   personnage ; le cap et
 *   l'inclinaison viennent de la rotation du contrôleur (la souris).
 * - La position de simulation est celle des **pieds** ; l'acteur (une capsule, centrée) est
 *   placé `FeetToActorOffset` plus haut.
 *
 * Connecté à un serveur (UOdysseeNetSubsystem, T027), il **prédit** : chaque pas est joué tout
 * de suite et envoyé au serveur, puis recalé sur ses snapshots (client::Prediction, la même que le
 * client de debug). Le joueur apparaît au point que donne le serveur. Hors ligne, il simule seul.
 */
UCLASS(ClassGroup = (Odyssee), meta = (BlueprintSpawnableComponent))
class ODYSSEENET_API UOdysseeMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseeMovementComponent();

	/** Distance entre les pieds et l'origine de l'acteur (demi-hauteur de la capsule), en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odyssee|Mouvement")
	float FeetToActorOffset = 96.0f;

	/** Déplacement voulu cette image : avant (+1) / arrière (-1), droite (+1) / gauche (-1). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void AddMoveInput(float Forward, float Right);

	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void SetJumpHeld(bool bHeld) { bJumpHeld = bHeld; }

	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void SetSprintHeld(bool bHeld) { bSprintHeld = bHeld; }

	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void SetCrouchHeld(bool bHeld) { bCrouchHeld = bHeld; }

	/** Action principale tenue (clic gauche : récolter, frapper) : InputButton::Primary. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void SetPrimaryHeld(bool bHeld) { bPrimaryHeld = bHeld; }

	/** Utiliser (touche E, T047) : mange ou boit le consommable tenu. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void SetUseHeld(bool bHeld) { bUseHeld = bHeld; }

	/** Case tenue de la barre d'outils (0 à 4) : l'outil qui récolte, l'objet qu'on mange. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void SetHeldSlot(int32 Slot);

	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	int32 GetHeldSlot() const { return HeldSlot; }

	/** Place les pieds du joueur à cet endroit (apparition, réapparition), vitesse nulle. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Mouvement")
	void TeleportFeetTo(FVector Feet);

	/** Vitesse en cm/s (animations). */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	FVector GetVelocity() const;

	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	bool IsGrounded() const;

	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	bool IsCrouched() const;

	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	bool IsSprinting() const;

	/** Endurance de 0 à 1 (HUD). */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	float GetStamina() const;

	/** Endurance vidée en courant : on ne peut plus courir jusqu'à ce qu'elle remonte. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	bool IsExhausted() const;

	/** Recalages sur le serveur depuis la connexion (HUD de debug). */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Mouvement")
	int32 GetCorrections() const;

	/** État complet de la simulation (C++ : client connecté, outils). */
	const odyssee::sim::PlayerState& GetSimState() const { return State; }

	/** La commande du prochain pas, telle que le serveur la recevra. */
	odyssee::net::InputCommand BuildCommand() const;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void StepSimulation();
	void StepOffline();
	void StepOnline(UOdysseeNetSubsystem& Net);
	void FollowServer(UOdysseeNetSubsystem& Net);
	void StartPrediction(const odyssee::net::Snapshot& Snapshot, odyssee::net::PlayerId Self);
	void ShowBetweenSteps(float Alpha);
	UOdysseeNetSubsystem* FindNet() const;

	odyssee::sim::PlayerState State;
	const odyssee::sim::Terrain* Terrain = nullptr;

	/** Positions des pieds affichées au pas d'avant et à celui-ci (1/256 m) : on interpole. */
	std::array<std::int32_t, 3> PreviousShown{};
	std::array<std::int32_t, 3> CurrentShown{};

	/** Prédiction de la partie en ligne en cours (absente hors ligne). */
	TUniquePtr<odyssee::client::Prediction> Prediction;
	uint32 PredictionSession = 0;  ///< connexion pour laquelle elle a été créée
	uint64 SeenSnapshot = 0;       ///< dernier snapshot pris en compte
	float Accumulated = 0.0f;

	FVector2D PendingMove = FVector2D::ZeroVector; ///< remis à zéro à chaque image
	FVector2D MoveOfStep = FVector2D::ZeroVector;  ///< dernier déplacement voulu, gardé pour le pas
	bool bJumpHeld = false;
	bool bSprintHeld = false;
	bool bCrouchHeld = false;
	bool bPrimaryHeld = false;
	bool bUseHeld = false;
	int32 HeldSlot = 0;
};
