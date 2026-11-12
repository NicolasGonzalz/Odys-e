#pragma once

#include "odyssee/world/WorldFile.hpp"

#include <cstdint>

namespace odyssee::world {

/// Géométrie d'un volume de collision (pavé ou cylindre vertical, tourné autour de son centre),
/// en entiers : le serveur et la prédiction du client doivent trancher pareil au bit près.

/// Vrai si le point (x, y) est dans l'empreinte au sol du volume, élargie de `clearance` de
/// chaque côté (le rayon du joueur : on teste son centre contre un volume grossi).
[[nodiscard]] bool covers(const CollisionBox& box, std::int32_t x, std::int32_t y,
                          std::int32_t clearance);

/// Bas et haut du volume, en 1/256 m.
[[nodiscard]] std::int32_t bottomOf(const CollisionBox& box);
[[nodiscard]] std::int32_t topOf(const CollisionBox& box);

/// Rayon d'un cercle centré sur le volume qui contient toute son empreinte (élargie), quelle que
/// soit sa rotation : sert à le ranger dans la grille.
[[nodiscard]] std::int64_t footprintRadius(const CollisionBox& box, std::int32_t clearance);

} // namespace odyssee::world
