#include "exploration/MapTables.hpp"

#include "resources/HarvestReach.hpp"
#include "resources/ResourceComponents.hpp"

#include <cstdlib>

namespace odyssee::server {

void MapTables::placeShelterTable(ecs::Registry& registry, const sim::Terrain& terrain,
                                  const std::array<std::int32_t, 3>& spawn) {
    const std::int32_t x = spawn[0] + kShelterTableOffset[0] * 256;
    const std::int32_t y = spawn[1] + kShelterTableOffset[1] * 256;
    const std::int32_t below = terrain.groundBelow(x, y, spawn[2] + 512);
    const std::int32_t z = below != sim::kNoGround ? below : terrain.groundHeight(x, y);

    const ecs::Entity table = registry.create();
    registry.add<Placement>(table, Placement{{x, y, z}, 0});
    registry.add<NetworkEntity>(table, NetworkEntity{static_cast<net::EntityId>(
                                           net::kFirstStationEntity + _tables.size())});
    _tables.push_back(table);
}

bool MapTables::isNear(const ecs::Registry& registry,
                       const std::array<std::int32_t, 3>& feet) const {
    const std::int64_t reach = kReach;
    for (const ecs::Entity table : _tables) {
        const Placement* placement = registry.get<Placement>(table);
        if (placement && horizontalDistanceSquared(feet, placement->position) <= reach * reach &&
            std::abs(std::int64_t{feet[2]} - placement->position[2]) <= kReachHeight) {
            return true;
        }
    }
    return false;
}

void MapTables::appendAll(const ecs::Registry& registry, std::vector<net::EntityState>& entities,
                          std::size_t limit) const {
    for (const ecs::Entity table : _tables) {
        const Placement* placement = registry.get<Placement>(table);
        const NetworkEntity* network = registry.get<NetworkEntity>(table);
        if (entities.size() >= limit) {
            return;
        }
        if (placement && network) {
            entities.push_back(net::EntityState{network->id, net::EntityKind::Station,
                                                placement->position, 0,
                                                net::kStationCartographyTable});
        }
    }
}

} // namespace odyssee::server
