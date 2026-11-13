#pragma once

#include "odyssee/data/DataError.hpp"
#include "odyssee/data/ItemDef.hpp"
#include "odyssee/data/RecipeDef.hpp"

#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

struct CatalogResult;

/// Tous les objets et toutes les recettes du jeu (data/items.txt et data/recipes.txt), chargés et
/// vérifiés : identifiants uniques, références connues, quantités dans les bornes. Le serveur et
/// le client Unreal lisent les mêmes fichiers avec ce même code.
class Catalog {
  public:
    /// Charge depuis du texte en mémoire (noms « items.txt » et « recipes.txt » dans les erreurs).
    [[nodiscard]] static CatalogResult fromText(std::string_view itemsText,
                                                std::string_view recipesText);
    /// Charge depuis deux fichiers sur le disque.
    [[nodiscard]] static CatalogResult fromFiles(const std::string& itemsPath,
                                                 const std::string& recipesPath);
    /// Charge items.txt et recipes.txt dans le dossier `dataDir`.
    [[nodiscard]] static CatalogResult fromDirectory(const std::string& dataDir);

    [[nodiscard]] std::span<const ItemDef> items() const { return _items; }
    [[nodiscard]] std::span<const RecipeDef> recipes() const { return _recipes; }

    /// Objet d'indice `id` (nullptr si hors limites).
    [[nodiscard]] const ItemDef* item(ItemId id) const;
    /// Indice de l'objet d'identifiant texte `key` (nullopt si inconnu).
    [[nodiscard]] std::optional<ItemId> findItem(std::string_view key) const;
    /// Recette d'identifiant texte `key` (nullptr si inconnue).
    [[nodiscard]] const RecipeDef* findRecipe(std::string_view key) const;
    /// Recettes fabricables à ce poste, dans l'ordre du fichier.
    [[nodiscard]] std::vector<const RecipeDef*> recipesAt(Station station) const;

  private:
    static CatalogResult build(std::string_view itemsText, std::string_view itemsFile,
                               std::string_view recipesText, std::string_view recipesFile);

    std::vector<ItemDef> _items;
    std::vector<RecipeDef> _recipes;
    std::map<std::string, ItemId, std::less<>> _itemsByKey;
};

struct CatalogResult {
    std::optional<Catalog> catalog; ///< vide en cas d'erreur
    std::optional<DataError> error;
};

} // namespace odyssee::data
