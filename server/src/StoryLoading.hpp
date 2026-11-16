#pragma once

#include "GameWorld.hpp"

#include <string>

namespace odyssee::server {

/// Donne au monde son histoire (T049) : lit story.txt du dossier `dataDir` (vide : `data` du
/// dossier courant) et pose les indices. Sans fichier, le monde reste sans indices ni survivants
/// (avertissement). Un fichier présent mais invalide est une erreur : la raison est dans le
/// journal et la fonction renvoie faux (le serveur refuse alors de démarrer).
[[nodiscard]] bool tellStory(GameWorld& world, const std::string& dataDir);

} // namespace odyssee::server
