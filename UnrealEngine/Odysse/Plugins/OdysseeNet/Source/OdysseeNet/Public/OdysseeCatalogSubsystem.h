// Objets, recettes et types de ressources côté client (T046) : les mêmes fichiers data/ que le
// serveur.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/ResourceCatalog.hpp"

#include <optional>

#include "OdysseeCatalogSubsystem.generated.h"

/**
 * Charge au lancement `items.txt`, `recipes.txt` et `resources.txt`, cherchés dans `<Projet>/Data/`
 * (jeu empaqueté) puis dans `data/` du dépôt (développement). Sert à afficher les noms et icônes de
 * l'inventaire, et la forme des ressources : le serveur n'envoie que des numéros (d'objet, de type
 * de ressource). Le fichier doit être le même que celui du serveur (l'ordre des types compte).
 */
UCLASS()
class ODYSSEENET_API UOdysseeCatalogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Le catalogue, ou nullptr s'il n'a pas pu être chargé. */
	const odyssee::data::Catalog* GetCatalog() const { return Catalog ? &*Catalog : nullptr; }

	/** Les types de ressources (data/resources.txt), ou nullptr s'ils n'ont pas pu être chargés. */
	const odyssee::data::ResourceCatalog* GetResourceTypes() const
	{
		return ResourceTypes ? &*ResourceTypes : nullptr;
	}

private:
	static FString FindDataDir();
	void LoadItems(const FString& Dir);
	void LoadResourceTypes(const FString& Dir);

	std::optional<odyssee::data::Catalog> Catalog;
	std::optional<odyssee::data::ResourceCatalog> ResourceTypes;
};
