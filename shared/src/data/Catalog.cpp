#include "odyssee/data/Catalog.hpp"

#include "ItemReader.hpp"
#include "RecipeReader.hpp"
#include "TextFile.hpp"
#include "odyssee/data/DataText.hpp"

#include <utility>

namespace odyssee::data {

namespace {

CatalogResult failure(DataError error) {
    return {std::nullopt, std::move(error)};
}

DataError unreadableFile(const std::string& path) {
    return DataError{path, 0,
                     "impossible de lire le fichier (absent, illisible ou plus de " +
                         std::to_string(kMaxDataFileSize / 1024) + " Kio)"};
}

} // namespace

CatalogResult Catalog::fromText(std::string_view itemsText, std::string_view recipesText) {
    return build(itemsText, "items.txt", recipesText, "recipes.txt");
}

CatalogResult Catalog::fromFiles(const std::string& itemsPath, const std::string& recipesPath) {
    const auto itemsText = readTextFile(itemsPath);
    if (!itemsText) {
        return failure(unreadableFile(itemsPath));
    }
    const auto recipesText = readTextFile(recipesPath);
    if (!recipesText) {
        return failure(unreadableFile(recipesPath));
    }
    return build(*itemsText, itemsPath, *recipesText, recipesPath);
}

CatalogResult Catalog::fromDirectory(const std::string& dataDir) {
    return fromFiles(dataDir + "/items.txt", dataDir + "/recipes.txt");
}

CatalogResult Catalog::build(std::string_view itemsText, std::string_view itemsFile,
                             std::string_view recipesText, std::string_view recipesFile) {
    const DataBlocks itemBlocks = parseDataText(itemsText, itemsFile);
    if (itemBlocks.error) {
        return failure(*itemBlocks.error);
    }
    ItemsRead items = readItems(itemBlocks.blocks, itemsFile);
    if (items.error) {
        return failure(*items.error);
    }

    const DataBlocks recipeBlocks = parseDataText(recipesText, recipesFile);
    if (recipeBlocks.error) {
        return failure(*recipeBlocks.error);
    }
    RecipesRead recipes = readRecipes(recipeBlocks.blocks, items, recipesFile);
    if (recipes.error) {
        return failure(*recipes.error);
    }

    Catalog catalog;
    catalog._items = std::move(items.items);
    catalog._itemsByKey = std::move(items.byKey);
    catalog._recipes = std::move(recipes.recipes);
    return {std::move(catalog), std::nullopt};
}

const ItemDef* Catalog::item(ItemId id) const {
    return id.value < _items.size() ? &_items[id.value] : nullptr;
}

std::optional<ItemId> Catalog::findItem(std::string_view key) const {
    const auto found = _itemsByKey.find(key);
    if (found == _itemsByKey.end()) {
        return std::nullopt;
    }
    return found->second;
}

const RecipeDef* Catalog::findRecipe(std::string_view key) const {
    for (const RecipeDef& recipe : _recipes) {
        if (recipe.key == key) {
            return &recipe;
        }
    }
    return nullptr;
}

std::vector<const RecipeDef*> Catalog::recipesAt(Station station) const {
    std::vector<const RecipeDef*> result;
    for (const RecipeDef& recipe : _recipes) {
        if (recipe.station == station) {
            result.push_back(&recipe);
        }
    }
    return result;
}

} // namespace odyssee::data
