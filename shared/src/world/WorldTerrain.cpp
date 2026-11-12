#include "odyssee/world/WorldTerrain.hpp"

#include "odyssee/world/CollisionShape.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace odyssee::world {

WorldTerrain::WorldTerrain(WorldFile world)
    : _world(std::move(world)),
      _grid(_world.boxes, _world.terrain.width(), _world.terrain.depth(), kPlayerRadius) {}

std::int32_t WorldTerrain::groundHeight(std::int32_t x, std::int32_t y) const {
    return groundBelow(x, y, std::numeric_limits<std::int32_t>::max());
}

std::int32_t WorldTerrain::groundBelow(std::int32_t x, std::int32_t y, std::int32_t z) const {
    std::int32_t ground = sim::kNoGround;
    if (const auto relief = reliefAt(x, y); relief && *relief <= z) {
        ground = *relief;
    }
    for (const std::uint32_t index : _grid.near(x, y)) {
        const CollisionBox& box = _world.boxes[index];
        const std::int32_t top = topOf(box);
        if (top <= z && top > ground && covers(box, x, y, kPlayerRadius)) {
            ground = top;
        }
    }
    return ground;
}

bool WorldTerrain::isSolid(std::int32_t x, std::int32_t y, std::int32_t bottom,
                           std::int32_t top) const {
    if (!isInside(x, y)) {
        return true; // bord de la carte
    }
    return std::ranges::any_of(_grid.near(x, y), [&](std::uint32_t index) {
        const CollisionBox& box = _world.boxes[index];
        return bottomOf(box) < top && topOf(box) > bottom && covers(box, x, y, kPlayerRadius);
    });
}

std::optional<std::int32_t> WorldTerrain::reliefAt(std::int32_t x, std::int32_t y) const {
    const HeightField& relief = _world.terrain;
    const auto col = static_cast<std::uint32_t>(
        std::clamp<std::int64_t>(x / std::int64_t{relief.quadSize}, 0, relief.quadsX - 1));
    const auto row = static_cast<std::uint32_t>(
        std::clamp<std::int64_t>(y / std::int64_t{relief.quadSize}, 0, relief.quadsY - 1));
    if (_world.isHole({col, row})) {
        return std::nullopt;
    }
    return relief.surfaceHeight(x, y);
}

bool WorldTerrain::isInside(std::int32_t x, std::int32_t y) const {
    return x >= 0 && y >= 0 && x <= _world.terrain.width() && y <= _world.terrain.depth();
}

std::optional<Marker> findPlayerSpawn(const WorldFile& world) {
    const auto spawn = std::ranges::find(world.markers, MarkerKind::PlayerSpawn, &Marker::kind);
    if (spawn == world.markers.end()) {
        return std::nullopt;
    }
    return *spawn;
}

} // namespace odyssee::world
