// Une ressource récoltable (arbre, rocher…), telle que le serveur la décrit (T044).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OdysseeResourceActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** Forme provisoire d'une ressource (data/resources.txt, `shape`), tant qu'il n'y a pas de modèle. */
UENUM(BlueprintType)
enum class EOdysseeResourceShape : uint8
{
	Unknown, ///< type inconnu ici (data/resources.txt différent de celui du serveur ?)
	Tree,
	Rock,
	Bush,
	Vein,
};

/**
 * Une ressource du monde, placée et mise à jour par UOdysseeNetSubsystem d'après le dernier
 * snapshot : elle est fixe, donc pas d'interpolation. L'origine de l'acteur est à sa **base**
 * (au sol).
 *
 * En attendant les modèles, elle s'affiche avec une forme de base d'Unreal selon son type
 * (cylindre pour un arbre, bloc pour un rocher, sphère pour un buisson, bloc aplati pour un filon),
 * plus petite et plus pâle une fois épuisée. Pour de vrais modèles : un Blueprint enfant, choisi
 * dans Paramètres du projet → Odyssee → Réseau, qui remplace les maillages par forme ou réagit à
 * OnResourceStateChanged.
 */
UCLASS(Blueprintable)
class ODYSSEENET_API AOdysseeResourceActor : public AActor
{
	GENERATED_BODY()

public:
	AOdysseeResourceActor();

	/** Numéro d'entité donné par le serveur. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Ressource")
	int32 EntityId = 0;

	/** Identifiant stable du type (`tree`, d'après data/resources.txt), vide si inconnu. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Ressource")
	FName ResourceKey;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Ressource")
	EOdysseeResourceShape Shape = EOdysseeResourceShape::Unknown;

	/** Épuisée : plus rien à récolter jusqu'à la repousse. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Ressource")
	bool bDepleted = false;

	/** Place la base de la ressource (cm). */
	void ShowAt(const FVector& BaseLocation);

	/** Change le type ou l'état ; ne fait rien s'ils sont les mêmes qu'avant. */
	void ShowState(FName NewKey, EOdysseeResourceShape NewShape, bool bNewDepleted);

	/** Appelé quand le type ou l'état change (première apparition comprise), après la mise à jour
	 *  de la forme provisoire. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Odyssee|Ressource")
	void OnResourceStateChanged();

protected:
	/** Formes provisoires ci-dessous. À décocher dans un Blueprint enfant qui affiche ses propres
	 *  modèles (dans OnResourceStateChanged) : la forme, la taille et la teinte ne sont plus
	 *  touchées. */
	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Ressource")
	bool bUseProvisionalShapes = true;

	/** Maillages provisoires par forme (taille 1 m, centrés : les formes de base de l'Engine). */
	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Ressource")
	TObjectPtr<UStaticMesh> TreeMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Ressource")
	TObjectPtr<UStaticMesh> RockMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Ressource")
	TObjectPtr<UStaticMesh> BushMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Ressource")
	TObjectPtr<UStaticMesh> VeinMesh;

	/** Matériau teinté par son paramètre `Color`. */
	UPROPERTY(EditDefaultsOnly, Category = "Odyssee|Ressource")
	TObjectPtr<UMaterialInterface> TintMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Anchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

private:
	void UpdateLook();
	UStaticMesh* MeshFor(EOdysseeResourceShape ForShape) const;
	void SetTint(const FLinearColor& Color);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Tint;

	bool bShown = false; ///< un état a déjà été montré (sinon, le premier compte comme un changement)
};
