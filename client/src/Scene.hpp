#pragma once

#include <raylib.h>

#include <array>
#include <cstdint>

namespace odyssee::debugclient {

/// Rendu 3D du client de debug (raylib), volontairement simple : un sol, des joueurs en blocs.
///
/// Repère : la simulation compte x vers l'est, y vers le sud et z vers le haut (1/256 m) ;
/// raylib met Y vers le haut. Le passage se fait ici seulement : X = x, Y = z, Z = y.

/// Position de simulation (1/256 m) → point raylib (mètres).
[[nodiscard]] Vector3 toWorld(const std::array<std::int32_t, 3>& position);

/// Interpolation entre deux points (rendu fluide entre deux ticks).
[[nodiscard]] Vector3 lerp(Vector3 from, Vector3 to, float t);

/// Caméra à hauteur des yeux du joueur (pieds en `feet`) ; en vue extérieure, recule derrière lui.
[[nodiscard]] Camera3D cameraFor(Vector3 feet, float yawDegrees, float pitchDegrees,
                                 bool thirdPerson);

/// Le sol provisoire autour de `around`, avec une grille tous les 2 m pour sentir le mouvement.
void drawGround(Vector3 around, float groundY);

/// Un joueur : un bloc de sa taille et un trait vers où il regarde.
void drawPlayer(Vector3 feet, float yawDegrees, Color color);

/// Couleur d'un joueur selon son numéro (0 à 3).
[[nodiscard]] Color playerColor(std::uint16_t id);

} // namespace odyssee::debugclient
