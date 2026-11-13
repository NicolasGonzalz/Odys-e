#pragma once

#include "GameWorld.hpp"
#include "resources/ResourceScatter.hpp"

#include <string>

namespace odyssee::server {

/// Peuple le monde de ses ressources récoltables (T044) : lit items.txt et resources.txt du
/// dossier `dataDir`, puis les place (`settings`).
///
/// `dataDir` vide : le dossier `data` du dossier courant ; s'il n'a pas de resources.txt, le
/// monde reste sans ressources (avertissement dans le journal). Des données présentes mais
/// invalides, ou un dossier demandé explicitement mais illisible, sont une erreur : la raison est
/// dans le journal et la fonction renvoie faux (le serveur refuse alors de démarrer).
[[nodiscard]] bool populateResources(GameWorld& world, const std::string& dataDir,
                                     const ScatterSettings& settings);

} // namespace odyssee::server
