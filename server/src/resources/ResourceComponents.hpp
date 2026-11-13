#pragma once

#include "odyssee/data/ResourceDef.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <array>
#include <cstdint>

namespace odyssee::server {

/// Composants des ressources récoltables (T044) et de ceux qui les récoltent : des données,
/// sans logique (règles dans Harvesting.hpp).

/// Une ressource posée dans le monde (arbre, rocher…). Pleine : `hitsLeft` coups avant
/// épuisement. Épuisée (`hitsLeft` à 0) : elle repousse, pleine, dans `regrowTicksLeft` ticks.
struct ResourceNode {
    data::ResourceTypeId type;
    std::uint16_t hitsLeft = 0;
    std::uint32_t regrowTicksLeft = 0;

    [[nodiscard]] bool depleted() const { return hitsLeft == 0; }
};

/// Place fixe d'une entité dans le monde (1/256 m, pieds ou base ; cap quantifié).
struct Placement {
    std::array<std::int32_t, 3> position{};
    std::uint16_t yaw = 0;
};

/// Numéro réseau d'une entité autre qu'un joueur (net::kFirstResourceEntity et au-delà).
struct NetworkEntity {
    net::EntityId id = 0;
};

/// Ce qui peut récolter (les joueurs) : ticks à attendre avant le prochain coup.
struct Harvester {
    std::uint32_t cooldownTicks = 0;
};

} // namespace odyssee::server
