#pragma once

#include "GameWorld.hpp"

#include <optional>
#include <string>

namespace odyssee::server {

/// Charge la carte d'un fichier monde (T041) : son sol, ses murs et son point d'apparition.
/// En cas d'échec (fichier absent ou invalide, pas de point d'apparition), explique pourquoi
/// dans le journal et renvoie std::nullopt.
[[nodiscard]] std::optional<GameWorld> loadGameWorld(const std::string& path);

} // namespace odyssee::server
