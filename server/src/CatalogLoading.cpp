#include "CatalogLoading.hpp"

#include "odyssee/core/Log.hpp"

#include <filesystem>
#include <system_error>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};

/// `data/` à côté de l'exécutable, sinon dans le dossier courant ; vide si aucun n'existe.
std::string findDefaultDataDir(const std::string& programPath) {
    std::error_code error;
    const auto besideProgram = std::filesystem::path(programPath).parent_path() / "data";
    for (const auto& candidate : {besideProgram, std::filesystem::path("data")}) {
        if (std::filesystem::is_regular_file(candidate / "items.txt", error)) {
            return candidate.string();
        }
    }
    return {};
}

} // namespace

std::string findDataDir(const std::string& requested, const std::string& programPath) {
    return requested.empty() ? findDefaultDataDir(programPath) : requested;
}

std::optional<data::Catalog> loadCatalog(const std::string& dir) {
    if (dir.empty()) {
        kLog.warn("dossier data/ introuvable : aucun objet ni recette (--data dossier)");
        return data::Catalog{};
    }
    auto result = data::Catalog::fromDirectory(dir);
    if (!result.catalog) {
        kLog.error("données « {} » non chargées : {}", dir,
                   result.error ? result.error->describe() : "erreur inconnue");
        return std::nullopt;
    }
    kLog.info("données « {} » chargées : {} objets, {} recettes", dir,
              result.catalog->items().size(), result.catalog->recipes().size());
    return std::move(result.catalog);
}

} // namespace odyssee::server
