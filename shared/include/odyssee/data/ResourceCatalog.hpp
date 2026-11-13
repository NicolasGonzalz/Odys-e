#pragma once

#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/DataError.hpp"
#include "odyssee/data/ResourceDef.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

struct ResourceCatalogResult;

/// Tous les types de ressources récoltables (data/resources.txt), chargés et vérifiés contre le
/// catalogue d'objets : objet récolté et outils connus, quantités dans les bornes.
class ResourceCatalog {
  public:
    /// Charge depuis du texte en mémoire (nom « resources.txt » dans les erreurs).
    [[nodiscard]] static ResourceCatalogResult fromText(std::string_view text,
                                                        const Catalog& items);
    /// Charge depuis un fichier sur le disque.
    [[nodiscard]] static ResourceCatalogResult fromFile(const std::string& path,
                                                        const Catalog& items);
    /// Charge resources.txt dans le dossier `dataDir`.
    [[nodiscard]] static ResourceCatalogResult fromDirectory(const std::string& dataDir,
                                                             const Catalog& items);

    [[nodiscard]] std::span<const ResourceDef> types() const { return _types; }

    /// Type d'indice `id` (nullptr si hors limites).
    [[nodiscard]] const ResourceDef* type(ResourceTypeId id) const;
    /// Type d'identifiant texte `key` (nullptr si inconnu).
    [[nodiscard]] const ResourceDef* find(std::string_view key) const;

  private:
    static ResourceCatalogResult build(std::string_view text, std::string_view file,
                                       const Catalog& items);

    std::vector<ResourceDef> _types;
};

struct ResourceCatalogResult {
    std::optional<ResourceCatalog> catalog; ///< vide en cas d'erreur
    std::optional<DataError> error;
};

} // namespace odyssee::data
