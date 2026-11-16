#pragma once

#include "ecs/Registry.hpp"

#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace odyssee::server {

/// Les tables de cartographie du monde (T074), entités de l'ECS (composants Placement et
/// NetworkEntity) : passer à côté reporte ses souvenirs sur la carte commune.
///
/// En attendant de pouvoir poser ses propres postes (construction), une seule table, fixe, dans
/// l'abri : quelques mètres à côté du point d'apparition.
class MapTables {
  public:
    /// Distance horizontale à laquelle on se sert d'une table (2,5 m), écart de hauteur toléré.
    static constexpr std::int32_t kReach = 640;
    static constexpr std::int32_t kReachHeight = 512;
    /// Place de la table de l'abri depuis le point d'apparition, en mètres (est, sud).
    static constexpr std::array<std::int32_t, 2> kShelterTableOffset{3, -2};

    /// Pose la table de l'abri, au sol (sous le plafond de l'abri s'il y en a un).
    void placeShelterTable(ecs::Registry& registry, const sim::Terrain& terrain,
                           const std::array<std::int32_t, 3>& spawn);

    /// Vrai si des pieds en `feet` sont à portée d'une table.
    [[nodiscard]] bool isNear(const ecs::Registry& registry,
                              const std::array<std::int32_t, 3>& feet) const;

    /// Ajoute les tables à `entities` (elles sont rares : toutes, sans tri), sans dépasser `limit`.
    void appendAll(const ecs::Registry& registry, std::vector<net::EntityState>& entities,
                   std::size_t limit) const;

  private:
    std::vector<ecs::Entity> _tables;
};

} // namespace odyssee::server
