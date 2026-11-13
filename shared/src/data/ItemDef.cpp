#include "odyssee/data/ItemDef.hpp"

#include <array>
#include <cstddef>

namespace odyssee::data {

namespace {

/// Noms des catégories dans les fichiers, dans l'ordre de l'énumération.
constexpr std::array<std::string_view, 7> kCategoryNames{
    "resource", "tool", "weapon", "ammo", "station", "consumable", "equipment",
};

} // namespace

std::optional<ItemCategory> parseItemCategory(std::string_view text) {
    for (std::size_t i = 0; i < kCategoryNames.size(); ++i) {
        if (kCategoryNames[i] == text) {
            return static_cast<ItemCategory>(i);
        }
    }
    return std::nullopt;
}

std::string_view itemCategoryName(ItemCategory category) {
    const auto index = static_cast<std::size_t>(category);
    return index < kCategoryNames.size() ? kCategoryNames[index] : std::string_view{};
}

} // namespace odyssee::data
