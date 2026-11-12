#include "WorldLoading.hpp"

#include "odyssee/core/Log.hpp"
#include "odyssee/world/WorldFileLoader.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <memory>
#include <utility>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};

} // namespace

std::optional<GameWorld> loadGameWorld(const std::string& path) {
    auto decoded = world::loadWorldFile(path);
    if (!decoded.world) {
        kLog.error("carte « {} » non chargée : {}", path, world::describe(decoded.error));
        return std::nullopt;
    }
    const auto marker = world::findPlayerSpawn(*decoded.world);
    if (!marker) {
        kLog.error("carte « {} » sans point d'apparition des joueurs", path);
        return std::nullopt;
    }

    auto terrain = std::make_unique<world::WorldTerrain>(std::move(*decoded.world));
    kLog.info("carte « {} » chargée : {} volumes de collision, apparition « {} »", path,
              terrain->world().boxes.size(), marker->id);
    return GameWorld(std::move(terrain), SpawnPoint{marker->position, marker->yaw});
}

} // namespace odyssee::server
