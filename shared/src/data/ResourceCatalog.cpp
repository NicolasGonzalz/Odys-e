#include "odyssee/data/ResourceCatalog.hpp"

#include "ResourceReader.hpp"
#include "TextFile.hpp"
#include "odyssee/data/DataText.hpp"

#include <algorithm>
#include <utility>

namespace odyssee::data {

namespace {

ResourceCatalogResult failure(DataError error) {
    return {std::nullopt, std::move(error)};
}

} // namespace

ResourceCatalogResult ResourceCatalog::fromText(std::string_view text, const Catalog& items) {
    return build(text, "resources.txt", items);
}

ResourceCatalogResult ResourceCatalog::fromFile(const std::string& path, const Catalog& items) {
    const auto text = readTextFile(path);
    if (!text) {
        return failure(DataError{path, 0,
                                 "impossible de lire le fichier (absent, illisible ou plus de " +
                                     std::to_string(kMaxDataFileSize / 1024) + " Kio)"});
    }
    return build(*text, path, items);
}

ResourceCatalogResult ResourceCatalog::fromDirectory(const std::string& dataDir,
                                                     const Catalog& items) {
    return fromFile(dataDir + "/resources.txt", items);
}

ResourceCatalogResult ResourceCatalog::build(std::string_view text, std::string_view file,
                                             const Catalog& items) {
    const DataBlocks blocks = parseDataText(text, file);
    if (blocks.error) {
        return failure(*blocks.error);
    }
    ResourcesRead read = readResources(blocks.blocks, items, file);
    if (read.error) {
        return failure(*read.error);
    }

    ResourceCatalog catalog;
    catalog._types = std::move(read.types);
    return {std::move(catalog), std::nullopt};
}

const ResourceDef* ResourceCatalog::type(ResourceTypeId id) const {
    return id.value < _types.size() ? &_types[id.value] : nullptr;
}

const ResourceDef* ResourceCatalog::find(std::string_view key) const {
    const auto found = std::find_if(_types.begin(), _types.end(),
                                    [key](const ResourceDef& type) { return type.key == key; });
    return found == _types.end() ? nullptr : &*found;
}

} // namespace odyssee::data
