#include "story/ClueField.hpp"

#include "resources/HarvestReach.hpp"
#include "resources/ResourceComponents.hpp"

#include <algorithm>
#include <utility>

namespace odyssee::server {

namespace {

constexpr std::int32_t kUnitsPerMeter = 256;
constexpr std::int32_t kTwoMeters = 2 * kUnitsPerMeter;

/// Le sol sous un indice. Zone 1 : celui de l'abri, sous le plafond éventuel (le point
/// d'apparition est au centre de stase) ; ailleurs, le dessus du relief.
std::int32_t groundFor(const data::ClueDef& clue, const sim::Terrain& terrain, std::int32_t x,
                       std::int32_t y, std::int32_t spawnZ) {
    if (clue.zone == 1) {
        const std::int32_t below = terrain.groundBelow(x, y, spawnZ + kTwoMeters);
        if (below != sim::kNoGround) {
            return below;
        }
    }
    return terrain.groundHeight(x, y);
}

} // namespace

std::size_t ClueField::place(ecs::Registry& registry, const data::StoryCatalog& story,
                             const sim::Terrain& terrain,
                             const std::array<std::int32_t, 3>& spawn) {
    for (const data::ClueDef& clue : story.clues()) {
        const std::int32_t x = spawn[0] + clue.offset[0] * kUnitsPerMeter;
        const std::int32_t y = spawn[1] + clue.offset[1] * kUnitsPerMeter;
        const ecs::Entity entity = registry.create();
        registry.add<ClueNode>(entity, ClueNode{clue.id});
        registry.add<Placement>(entity,
                                Placement{{x, y, groundFor(clue, terrain, x, y, spawn[2])}, 0});
        registry.add<NetworkEntity>(entity, NetworkEntity{static_cast<net::EntityId>(
                                                net::kFirstClueEntity + clue.id.value)});
        _clues.push_back(entity);
    }
    return _clues.size();
}

std::optional<data::ClueId> ClueField::target(const ecs::Registry& registry,
                                              const std::array<std::int32_t, 3>& feet,
                                              std::uint16_t yaw) const {
    std::optional<data::ClueId> best;
    std::int64_t bestDistance = 0;
    for (const ecs::Entity entity : _clues) {
        const Placement* placement = registry.get<Placement>(entity);
        const ClueNode* node = registry.get<ClueNode>(entity);
        if (!placement || !node || !isWithinReach(feet, yaw, placement->position)) {
            continue;
        }
        const std::int64_t distance = horizontalDistanceSquared(feet, placement->position);
        if (!best || distance < bestDistance) {
            best = node->clue;
            bestDistance = distance;
        }
    }
    return best;
}

void ClueField::appendNearby(const ecs::Registry& registry,
                             const std::array<std::int32_t, 3>& around,
                             const std::function<bool(data::ClueId)>& read,
                             std::vector<net::EntityState>& entities, std::size_t limit) const {
    // Les plus proches d'abord ; à distance égale, l'ordre de pose (déterministe).
    std::vector<std::pair<std::int64_t, std::size_t>> inRange;
    const std::int64_t radiusSquared = std::int64_t{kSnapshotRadius} * kSnapshotRadius;
    for (std::size_t i = 0; i < _clues.size(); ++i) {
        const Placement* placement = registry.get<Placement>(_clues[i]);
        const std::int64_t distance =
            placement ? horizontalDistanceSquared(around, placement->position) : radiusSquared + 1;
        if (distance <= radiusSquared) {
            inRange.emplace_back(distance, i);
        }
    }
    std::sort(inRange.begin(), inRange.end());

    for (const auto& [distance, index] : inRange) {
        const ecs::Entity entity = _clues[index];
        const NetworkEntity* network = registry.get<NetworkEntity>(entity);
        const Placement* placement = registry.get<Placement>(entity);
        const ClueNode* node = registry.get<ClueNode>(entity);
        if (entities.size() >= limit) {
            return;
        }
        if (network && placement && node) {
            const auto flags = static_cast<std::uint8_t>(read(node->clue) ? net::kClueRead : 0);
            entities.push_back(net::EntityState{network->id, net::EntityKind::Clue,
                                                placement->position, 0, flags});
        }
    }
}

} // namespace odyssee::server
