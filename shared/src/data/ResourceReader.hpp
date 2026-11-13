#pragma once

// Lecture interne de data/resources.txt : blocs « [resource …] » → ResourceDef, avec vérification
// des objets cités (catalogue d'objets chargé avant).

#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/DataError.hpp"
#include "odyssee/data/DataText.hpp"
#include "odyssee/data/ResourceDef.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace odyssee::data {

inline constexpr std::uint32_t kMaxResourceHits = 1000;
inline constexpr std::uint32_t kMaxHitTicks = 20 * 10;              ///< 10 secondes
inline constexpr std::uint32_t kMaxRegrowTicks = 20 * 60 * 60 * 24; ///< 24 heures
inline constexpr std::uint32_t kMaxAbundance = 1000;
inline constexpr std::size_t kMaxResourceTools = 8;

struct ResourcesRead {
    std::vector<ResourceDef> types; ///< indice = ResourceTypeId
    std::optional<DataError> error;
};

/// Interprète les blocs de ressources ; `items` donne les objets connus, `file` sert aux erreurs.
[[nodiscard]] ResourcesRead readResources(const std::vector<DataBlock>& blocks,
                                          const Catalog& items, std::string_view file);

} // namespace odyssee::data
