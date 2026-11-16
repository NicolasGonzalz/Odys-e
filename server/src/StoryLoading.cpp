#include "StoryLoading.hpp"

#include "odyssee/core/Log.hpp"
#include "odyssee/data/StoryCatalog.hpp"

#include <filesystem>
#include <system_error>
#include <utility>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"histoire"};

} // namespace

bool tellStory(GameWorld& world, const std::string& dataDir) {
    world.addShelterMapTable(); // T074 : la table de cartographie de l'abri
    const std::string dir = dataDir.empty() ? std::string("data") : dataDir;
    std::error_code ignored;
    if (!std::filesystem::exists(dir + "/story.txt", ignored)) {
        kLog.warn("pas de « {}/story.txt » : monde sans indices ni survivants", dir);
        return true;
    }
    data::StoryCatalogResult story = data::StoryCatalog::fromDirectory(dir);
    if (!story.catalog) {
        kLog.error("histoire non chargée : {}", story.error->describe());
        return false;
    }
    const std::size_t survivors = story.catalog->survivors().size();
    const std::size_t placed = world.useStory(std::move(*story.catalog));
    kLog.info("histoire : {} survivant(s), {} indice(s) posé(s)", survivors, placed);
    return true;
}

} // namespace odyssee::server
