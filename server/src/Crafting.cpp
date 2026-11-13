#include "Crafting.hpp"

namespace odyssee::server {

namespace {

bool hasIngredients(const Inventory& inventory, const data::RecipeDef& recipe) {
    for (const data::ItemStack& input : recipe.inputs) {
        if (countItem(inventory, input.item) < input.count) {
            return false;
        }
    }
    return true;
}

} // namespace

net::CraftOutcome startCraft(Crafting& crafting, Inventory& inventory, std::uint16_t recipe,
                             const data::Catalog& catalog) {
    if (recipe >= catalog.recipes().size()) {
        return net::CraftOutcome::UnknownRecipe;
    }
    const data::RecipeDef& def = catalog.recipes()[recipe];
    if (crafting.ticksLeft > 0) {
        return net::CraftOutcome::Busy;
    }
    if (def.station != data::Station::Hand) {
        return net::CraftOutcome::NeedStation;
    }
    if (!hasIngredients(inventory, def)) {
        return net::CraftOutcome::MissingIngredients;
    }
    for (const data::ItemStack& input : def.inputs) {
        (void)removeItem(inventory, input.item, input.count); // assez : vérifié juste avant
    }
    crafting = Crafting{recipe, def.ticks};
    return net::CraftOutcome::Started;
}

std::optional<std::uint16_t> stepCraft(Crafting& crafting, Inventory& inventory,
                                       const data::Catalog& catalog) {
    if (crafting.ticksLeft == 0 || --crafting.ticksLeft > 0) {
        return std::nullopt;
    }
    if (crafting.recipe < catalog.recipes().size()) {
        const data::ItemStack& output = catalog.recipes()[crafting.recipe].output;
        (void)addItem(inventory, output.item, output.count, catalog);
    }
    return crafting.recipe;
}

} // namespace odyssee::server
