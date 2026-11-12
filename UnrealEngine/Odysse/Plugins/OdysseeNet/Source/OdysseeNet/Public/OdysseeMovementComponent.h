// Mouvement du joueur par la simulation partagée (T016), à la place du CharacterMovementComponent.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"

#include "OdysseeMovementComponent.generated.h"

/**
 * Déplace son acteur avec `odyssee::sim::step`, la même fonction que le serveur, sur le sol et
 * les murs du fichier monde (UOdysseeWorldSubsystem) : pas de physique Unreal (ADR 0003).
 *
 * - La simulation avance à pas fixe, 20 fois par seconde comme le serveur. Entre deux pas,
 *   l'affichage interpole : la caméra reste fluide à 60 images/s et plus.
 * - Les entrées (déplacement, saut, course, accroupi) sont données par le personnage ; le cap et
 *   l'inclinaison viennent de la rotation du contrôleur (la souris).
 * - La position de simulation est celle des **pieds** ; l'acteur (une capsule, centrée) est
 *   placé `FeetToActorOffset` plus haut.
 *
 * Hors ligne pour l'instant ; le client connecté (T027) enverra au serveur les commandes de ce
 * composant et le recalera sur ses snapshots.
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

	/** État complet de la simulation (C++ : client connecté, outils). */
	const odyssee::sim::PlayerState& GetSimState() const { return State; }

	/** La commande du prochain pas, telle que le serveur la recevra. */
	odyssee::net::InputCommand BuildCommand() const;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void StepSimulation();
	void ShowBetweenSteps(float Alpha);
	FVector ToFeetLocation(const odyssee::sim::PlayerState& Of) const;

	odyssee::sim::PlayerState State;
	odyssee::sim::PlayerState Previous; ///< état du pas d'avant : l'affichage interpole entre les deux
	const odyssee::sim::Terrain* Terrain = nullptr;
	float Accumulated = 0.0f;

	FVector2D PendingMove = FVector2D::ZeroVector; ///< remis à zéro à chaque image
	FVector2D MoveOfStep = FVector2D::ZeroVector;  ///< dernier déplacement voulu, gardé pour le pas
	bool bJumpHeld = false;
	bool bSprintHeld = false;
	bool bCrouchHeld = false;
};
