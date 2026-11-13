// Objets et recettes du jeu côté client (T046) : les mêmes fichiers data/ que le serveur.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "odyssee/data/Catalog.hpp"

#include <optional>

#include "OdysseeCatalogSubsystem.generated.h"

/**
 * Charge au lancement `items.txt` et `recipes.txt`, cherchés dans `<Projet>/Data/` (jeu empaqueté)
 * puis dans `data/` du dépôt (développement). Sert à afficher les noms et icônes de l'inventaire :
 * le serveur n'envoie que des numéros d'objet.
 */
UCLASS()
class ODYSSEENET_API UOdysseeCatalogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Le catalogue, ou nullptr s'il n'a pas pu être chargé. */
	const odyssee::data::Catalog* GetCatalog() const { return Catalog ? &*Catalog : nullptr; }

private:
	static FString FindDataDir();

	std::optional<odyssee::data::Catalog> Catalog;
};
