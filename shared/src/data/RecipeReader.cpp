#include "RecipeReader.hpp"

#include "DataValues.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <utility>

namespace odyssee::data {

namespace {

constexpr std::array<std::string_view, 4> kAllowedKeys{"station", "input", "output", "ticks"};
constexpr std::array<std::string_view, 1> kRepeatableKeys{"input"};
constexpr std::array<std::string_view, 4> kRequiredKeys{"station", "input", "output", "ticks"};

using RecipeKeys = std::set<std::string, std::less<>>;

/// Lit un bloc dont les clés sont déjà vérifiées. Erreur, ou nullopt si `recipe` est remplie.
class RecipeBlockReader {
  public:
    RecipeBlockReader(const DataBlock& block, const ItemsRead& items, std::string_view file)
        : _block(block), _items(items), _file(file) {}

    std::optional<DataError> read(RecipeDef& recipe) const {
        if (auto error = readStation(recipe)) {
            return error;
        }
        if (auto error = readInputs(recipe)) {
            return error;
        }
        if (auto error = readOutput(recipe)) {
            return error;
        }
        return readTicks(recipe);
    }

  private:
    std::optional<DataError> readStation(RecipeDef& recipe) const {
        const DataField& field = *findField(_block, "station");
        const auto station = parseStation(field.value);
        if (!station) {
            return fail(field,
                        "poste inconnu " + quoted(field.value) + " (hand, workbench, forge)");
        }
        recipe.station = *station;
        return std::nullopt;
    }

    std::optional<DataError> readInputs(RecipeDef& recipe) const {
        for (const DataField& field : _block.fields) {
            if (field.key != "input") {
                continue;
            }
            ItemStack input;
            if (auto error = readStack(field, input)) {
                return error;
            }
            if (auto error = addInput(field, input, recipe)) {
                return error;
            }
        }
        return std::nullopt;
    }

    std::optional<DataError> addInput(const DataField& field, const ItemStack& input,
                                      RecipeDef& recipe) const {
        const bool repeated =
            std::any_of(recipe.inputs.begin(), recipe.inputs.end(),
                        [&input](const ItemStack& other) { return other.item == input.item; });
        if (repeated) {
            return fail(field, "ingrédient répété : regrouper les quantités sur une seule ligne");
        }
        if (recipe.inputs.size() >= kMaxRecipeInputs) {
            return fail(field,
                        "trop d'ingrédients (maximum " + std::to_string(kMaxRecipeInputs) + ")");
        }
        recipe.inputs.push_back(input);
        return std::nullopt;
    }

    std::optional<DataError> readOutput(RecipeDef& recipe) const {
        const DataField& field = *findField(_block, "output");
        if (auto error = readStack(field, recipe.output)) {
            return error;
        }
        const std::uint16_t maxStack = _items.items[recipe.output.item.value].maxStack;
        if (recipe.output.count > maxStack) {
            return fail(field, "quantité produite supérieure à la pile max de l'objet (" +
                                   std::to_string(maxStack) + ")");
        }
        return std::nullopt;
    }

    std::optional<DataError> readTicks(RecipeDef& recipe) const {
        const DataField& field = *findField(_block, "ticks");
        const auto ticks = parseNumber(field.value, 1, kMaxCraftTicks);
        if (!ticks) {
            return fail(field, "durée " + quoted(field.value) + " invalide : entier de 1 à " +
                                   std::to_string(kMaxCraftTicks) + " ticks attendu");
        }
        recipe.ticks = *ticks;
        return std::nullopt;
    }

    /// « identifiant quantité » (`stone 3`) → objet connu et quantité dans les bornes.
    std::optional<DataError> readStack(const DataField& field, ItemStack& stack) const {
        const std::size_t space = field.value.find_first_of(" \t");
        if (space == std::string_view::npos) {
            return fail(field, "« identifiant quantité » attendu (ex. « stone 3 »)");
        }
        const std::string_view key = field.value.substr(0, space);
        // La valeur est sans blanc final (parseDataText) : il reste au moins un caractère.
        const std::string_view countText =
            field.value.substr(field.value.find_first_not_of(" \t", space));
        const auto found = _items.byKey.find(key);
        if (found == _items.byKey.end()) {
            return fail(field, "objet inconnu " + quoted(key) + " (absent de items.txt)");
        }
        const auto count = parseNumber(countText, 1, kMaxStackSize);
        if (!count) {
            return fail(field, "quantité " + quoted(countText) + " invalide : entier de 1 à " +
                                   std::to_string(kMaxStackSize) + " attendu");
        }
        stack = ItemStack{found->second, static_cast<std::uint16_t>(*count)};
        return std::nullopt;
    }

    DataError fail(const DataField& field, std::string message) const {
        return makeError(_file, field.line, std::move(message));
    }

    const DataBlock& _block;
    const ItemsRead& _items;
    std::string_view _file;
};

/// En-tête et clés du bloc ; erreur, ou nullopt si on peut le lire.
std::optional<DataError> checkRecipeBlock(const DataBlock& block, const RecipeKeys& known,
                                          std::string_view file) {
    if (block.kind != "recipe") {
        return makeError(file, block.line,
                         "type de bloc " + quoted(block.kind) + " inattendu : « recipe » attendu");
    }
    if (!isValidKey(block.id)) {
        return makeError(file, block.line,
                         "identifiant " + quoted(block.id) +
                             " invalide (minuscules, chiffres et « _ », ex. « stone_axe »)");
    }
    if (known.contains(block.id)) {
        return makeError(file, block.line, "recette " + quoted(block.id) + " déjà définie");
    }
    return checkBlockKeys(block, {kAllowedKeys, kRepeatableKeys, kRequiredKeys}, file);
}

} // namespace

RecipesRead readRecipes(const std::vector<DataBlock>& blocks, const ItemsRead& items,
                        std::string_view file) {
    RecipesRead result;
    RecipeKeys known;
    for (const DataBlock& block : blocks) {
        if (auto error = checkRecipeBlock(block, known, file)) {
            result.error = std::move(error);
            return result;
        }

        RecipeDef recipe;
        recipe.key = std::string(block.id);
        if (auto error = RecipeBlockReader(block, items, file).read(recipe)) {
            result.error = std::move(error);
            return result;
        }
        known.insert(recipe.key);
        result.recipes.push_back(std::move(recipe));
    }
    return result;
}

} // namespace odyssee::data
