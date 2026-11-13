#pragma once

#include "odyssee/data/Catalog.hpp"

#include <optional>
#include <string>

namespace odyssee::server {

/// Le dossier des données : `requested` s'il est donné (--data), sinon `data/` à côté de
/// l'exécutable (`programPath`), sinon `data/` du dossier courant ; vide si aucun n'existe.
[[nodiscard]] std::string findDataDir(const std::string& requested, const std::string& programPath);

/// Charge les objets et recettes (T046) du dossier `dataDir`. Vide : catalogue vide
/// (avertissement). Données invalides : l'erreur est journalisée (fichier, ligne) et le résultat
/// est std::nullopt.
[[nodiscard]] std::optional<data::Catalog> loadCatalog(const std::string& dataDir);

} // namespace odyssee::server
