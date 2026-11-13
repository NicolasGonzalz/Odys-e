#pragma once

#include "odyssee/data/ItemDef.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

/// Où se fabrique une recette (docs/GAMEPLAY.md §2).
enum class Station : std::uint8_t {
    Hand,      ///< partout, sans poste
    Workbench, ///< établi
    Forge,
};

/// « hand », « workbench », « forge » → poste (nullopt si inconnu).
[[nodiscard]] std::optional<Station> parseStation(std::string_view text);
[[nodiscard]] std::string_view stationName(Station station);

/// Une quantité d'un objet.
struct ItemStack {
    ItemId item;
    std::uint16_t count = 1;

    friend bool operator==(const ItemStack&, const ItemStack&) = default;
};

/// Une recette, telle que lue dans data/recipes.txt.
struct RecipeDef {
    std::string key; ///< identifiant texte stable : `stone_axe`
    Station station = Station::Hand;
    std::vector<ItemStack> inputs; ///< ingrédients, tous différents, au moins un
    ItemStack output;
    std::uint32_t ticks = 0; ///< durée de fabrication (20 ticks = 1 s)
};

} // namespace odyssee::data
