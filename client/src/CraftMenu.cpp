#include "CraftMenu.hpp"

#include <raylib.h>

namespace odyssee::debugclient {

namespace {

constexpr int kLine = 22;

std::string ingredientsOf(const data::RecipeDef& recipe, const data::Catalog& catalog) {
    std::string text;
    for (const data::ItemStack& input : recipe.inputs) {
        const data::ItemDef* item = catalog.item(input.item);
        text += (text.empty() ? "" : ", ") + std::to_string(input.count) + " " +
                (item ? item->name : "?");
    }
    return text;
}

std::string outcomeText(net::CraftOutcome outcome) {
    switch (outcome) {
    case net::CraftOutcome::Started:
        return "en cours";
    case net::CraftOutcome::Done:
        return "terminé";
    case net::CraftOutcome::Busy:
        return "refusé : une fabrication est déjà en cours";
    case net::CraftOutcome::NeedStation:
        return "refusé : il faut un établi ou une forge";
    case net::CraftOutcome::MissingIngredients:
        return "refusé : ingrédients manquants";
    case net::CraftOutcome::UnknownRecipe:
        return "refusé : recette inconnue";
    }
    return "?";
}

} // namespace

std::optional<std::uint16_t> CraftMenu::handleKeys(const data::Catalog& catalog) {
    const std::size_t count = catalog.recipes().size();
    if (!_open || count == 0) {
        return std::nullopt;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        _cursor = (_cursor + 1) % count;
    }
    if (IsKeyPressed(KEY_UP)) {
        _cursor = (_cursor + count - 1) % count;
    }
    _cursor = _cursor < count ? _cursor : 0;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        return static_cast<std::uint16_t>(_cursor);
    }
    return std::nullopt;
}

void CraftMenu::onStatus(const net::CraftStatus& status, const data::Catalog& catalog) {
    const auto recipes = catalog.recipes();
    const std::string name = status.recipe < recipes.size()
                                 ? recipes[status.recipe].key
                                 : "recette " + std::to_string(status.recipe);
    _message = "Fabrication de " + name + " : " + outcomeText(status.outcome);
    if (status.outcome == net::CraftOutcome::Started) {
        _message += " (" + std::to_string(status.ticks / 20) + " s)";
    }
}

void CraftMenu::draw(const data::Catalog& catalog) const {
    if (!_open) {
        return;
    }
    const int x = GetScreenWidth() - 560;
    DrawRectangle(x - 12, 12, 556, GetScreenHeight() - 24, Color{0, 0, 0, 170});
    DrawText("Artisanat - haut/bas, Entrée : fabriquer, C : fermer", x, 20, 18, RAYWHITE);
    const auto recipes = catalog.recipes();
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        const data::RecipeDef& recipe = recipes[i];
        const data::ItemDef* output = catalog.item(recipe.output.item);
        const std::string station = recipe.station == data::Station::Hand        ? ""
                                    : recipe.station == data::Station::Workbench ? " [établi]"
                                                                                 : " [forge]";
        const std::string line =
            (output ? output->name : recipe.key) + station + " : " + ingredientsOf(recipe, catalog);
        const int y = 50 + static_cast<int>(i) * kLine;
        DrawText(line.c_str(), x, y, 16, i == _cursor ? ORANGE : LIGHTGRAY);
    }
}

} // namespace odyssee::debugclient
