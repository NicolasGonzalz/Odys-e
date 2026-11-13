#pragma once

#include "odyssee/data/ResourceDef.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace odyssee::server {

/// Placement des ressources récoltables (T044), en attendant les marqueurs posés dans Unreal
/// (T038) : tirage **déterministe** (graine fixe, entiers seulement) dans un anneau autour du
/// point d'apparition. Même terrain, même graine, mêmes types : mêmes ressources, partout.
///
/// Un emplacement n'est retenu que sur le relief, à découvert : sol présent (pas une case
/// percée), aucun volume solide entre juste sous le sol et kClearance au-dessus (ni mur, ni
/// plafond, ni toit ou plancher de bâtiment), pente douce (le sol à 1 m autour ne s'écarte pas de
/// plus de kMaxRise), et assez loin des ressources déjà placées.
struct ScatterSettings {
    std::uint64_t seed = 0;
    std::size_t count = 0;       ///< ressources voulues (moins si la place manque)
    std::int32_t minRadius = 0;  ///< 1/256 m, autour du centre
    std::int32_t maxRadius = 0;  ///< 1/256 m
    std::int32_t minSpacing = 0; ///< 1/256 m entre deux ressources
    std::uint32_t attemptsPerNode = 30;
};

/// Sur la carte : de 15 à 130 m du point d'apparition, ~160 ressources, 4 m d'écart au moins.
inline constexpr ScatterSettings kMapScatter{.seed = 0x0D755EE0044ULL,
                                             .count = 160,
                                             .minRadius = 15 * 256,
                                             .maxRadius = 130 * 256,
                                             .minSpacing = 4 * 256,
                                             .attemptsPerNode = 30};
/// Sur le sol plat provisoire (serveur lancé sans carte) : une douzaine, de 6 à 25 m.
inline constexpr ScatterSettings kProvisionalScatter{.seed = 0x0D755EE0044ULL,
                                                     .count = 12,
                                                     .minRadius = 6 * 256,
                                                     .maxRadius = 25 * 256,
                                                     .minSpacing = 3 * 256,
                                                     .attemptsPerNode = 30};

inline constexpr std::int32_t kClearance = 3 * 256; ///< 3 m dégagés au-dessus du sol
inline constexpr std::int32_t kMaxRise = 154;       ///< 60 cm sur 1 m : pente de 31° au plus

/// Une ressource à poser.
struct PlacedResource {
    data::ResourceTypeId type;
    std::array<std::int32_t, 3> position{}; ///< base, posée sur le sol
    std::uint16_t yaw = 0;                  ///< rotation pour varier l'aspect

    friend bool operator==(const PlacedResource&, const PlacedResource&) = default;
};

/// Tire les ressources autour de `center` (x et y), types choisis selon leur abondance.
[[nodiscard]] std::vector<PlacedResource>
scatterResources(const sim::Terrain& terrain, const std::array<std::int32_t, 3>& center,
                 std::span<const data::ResourceDef> types, const ScatterSettings& settings);

} // namespace odyssee::server
