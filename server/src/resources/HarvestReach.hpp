#pragma once

#include <array>
#include <cstdint>

namespace odyssee::server {

/// Ce qu'un joueur peut frapper (T044) : une ressource **à portée** et **devant lui**, d'après
/// son cap (l'inclinaison de la caméra est ignorée). Positions en 1/256 m, x est, y sud, z haut.

inline constexpr std::int32_t kHarvestReach = 640;  ///< 2,5 m à l'horizontale, pieds → base
inline constexpr std::int32_t kHarvestHeight = 640; ///< 2,5 m d'écart de hauteur au plus
/// Demi-angle du cône devant le joueur : 45° de part et d'autre de son cap.
inline constexpr double kHarvestConeHalfAngleDegrees = 45.0;
/// Plus près que ça (40 cm), la ressource est frappée quel que soit le cap : collé à elle, la
/// direction n'a plus de sens.
inline constexpr std::int32_t kPointBlank = 102;

/// Carré de la distance horizontale entre deux points (1/256 m au carré).
[[nodiscard]] std::int64_t horizontalDistanceSquared(const std::array<std::int32_t, 3>& a,
                                                     const std::array<std::int32_t, 3>& b);

/// Vrai si un joueur aux pieds en `feet`, de cap `yaw` (net::quantizeYaw), peut frapper la
/// ressource posée en `target`.
[[nodiscard]] bool isWithinReach(const std::array<std::int32_t, 3>& feet, std::uint16_t yaw,
                                 const std::array<std::int32_t, 3>& target);

} // namespace odyssee::server
