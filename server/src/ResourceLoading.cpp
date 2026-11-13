#include "ResourceLoading.hpp"

#include "odyssee/core/Log.hpp"
#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/ResourceCatalog.hpp"

#include <filesystem>
#include <system_error>
#include <utility>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};
constexpr const char* kDefaultDataDir = "data";

bool hasResourceFile(const std::string& dataDir) {
    std::error_code error;
    return std::filesystem::is_regular_file(dataDir + "/resources.txt", error);
}

} // namespace

bool populateResources(GameWorld& world, const std::string& dataDir,
                       const ScatterSettings& settings) {
    const std::string dir = dataDir.empty() ? kDefaultDataDir : dataDir;
    if (dataDir.empty() && !hasResourceFile(dir)) {
        kLog.warn("pas de « {}/resources.txt » : monde sans ressources (voir --data)", dir);
        return true;
    }

    const data::CatalogResult items = data::Catalog::fromDirectory(dir);
    if (!items.catalog) {
        kLog.error("données non chargées : {}", items.error->describe());
        return false;
    }
    data::ResourceCatalogResult types = data::ResourceCatalog::fromDirectory(dir, *items.catalog);
    if (!types.catalog) {
        kLog.error("données non chargées : {}", types.error->describe());
        return false;
    }

    const std::size_t typeCount = types.catalog->types().size();
    const std::size_t created = world.addResources(std::move(*types.catalog), settings);
    kLog.info("{} ressources récoltables placées ({} types, « {} »)", created, typeCount, dir);
    return true;
}

} // namespace odyssee::server
