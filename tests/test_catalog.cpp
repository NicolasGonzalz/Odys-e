// Catalogue des objets et recettes (data/items.txt, data/recipes.txt, docs/DATA_FORMAT.md).
#include "odyssee/data/Catalog.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>

using namespace odyssee::data;

namespace {

const std::string kDataDir = std::string(ODYSSEE_SOURCE_DIR) + "/data";

constexpr std::string_view kItems = R"(# deux objets
[item stick]
name = Bâton
stack = 40
category = resource

[item stone_axe]
name = Hache de pierre
stack = 1
category = tool
model = outils/hache_pierre
)";

constexpr std::string_view kRecipeHead = "[recipe stone_axe]\nstation = hand\n";

/// Erreur attendue au chargement (échec du test si le chargement réussit).
DataError loadError(std::string_view items, std::string_view recipes) {
    CatalogResult result = Catalog::fromText(items, recipes);
    REQUIRE_FALSE(result.catalog.has_value());
    REQUIRE(result.error.has_value());
    return *result.error;
}

DataError itemsError(std::string_view items) {
    return loadError(items, "");
}

/// Recette `stone_axe` (à la main) avec ces lignes d'ingrédients, ce résultat et cette durée.
/// Lignes : 1 en-tête, 2 poste, puis ingrédients, résultat et durée.
DataError recipeError(std::string_view inputLines, std::string_view output = "stone_axe 1",
                      std::string_view ticks = "60") {
    return loadError(kItems, std::string(kRecipeHead) + std::string(inputLines) + "output = " +
                                 std::string(output) + "\nticks = " + std::string(ticks) + "\n");
}

const char* const kValidRecipeBody = "input = stick 2\noutput = stone_axe 1\nticks = 60\n";

} // namespace

TEST_CASE("Catalogue : les vrais fichiers data/ se chargent") {
    const CatalogResult result = Catalog::fromDirectory(kDataDir);
    REQUIRE_MESSAGE(result.catalog.has_value(), (result.error ? result.error->describe() : ""));
    const Catalog& catalog = *result.catalog;

    CHECK(catalog.items().size() >= 40);
    CHECK(catalog.recipes().size() >= 20);

    const auto axe = catalog.findItem("stone_axe");
    REQUIRE(axe.has_value());
    CHECK(catalog.item(*axe)->name == "Hache de pierre");
    CHECK(catalog.item(*axe)->category == ItemCategory::Tool);
    CHECK(catalog.item(*axe)->model == "outils/hache_pierre");

    // Les postes de GAMEPLAY §2 : l'établi à la main, la forge à l'établi, les lingots à la forge.
    REQUIRE(catalog.findRecipe("workbench") != nullptr);
    CHECK(catalog.findRecipe("workbench")->station == Station::Hand);
    CHECK(catalog.findRecipe("forge")->station == Station::Workbench);
    CHECK(catalog.findRecipe("plank")->station == Station::Workbench);
    CHECK(catalog.findRecipe("ingot")->station == Station::Forge);
    for (const Station station : {Station::Hand, Station::Workbench, Station::Forge}) {
        CHECK_FALSE(catalog.recipesAt(station).empty());
    }
}

TEST_CASE("Catalogue : chaque modèle cité existe dans art/exports") {
    const CatalogResult result = Catalog::fromDirectory(kDataDir);
    REQUIRE(result.catalog.has_value());
    const std::filesystem::path exports = std::filesystem::path(ODYSSEE_SOURCE_DIR) / "art/exports";
    for (const ItemDef& item : result.catalog->items()) {
        if (!item.model.empty()) {
            CHECK_MESSAGE(std::filesystem::exists(exports / (item.model + ".fbx")), item.key);
        }
    }
}

TEST_CASE("Catalogue : indices compacts et recherches") {
    const CatalogResult result =
        Catalog::fromText(kItems, std::string(kRecipeHead) + kValidRecipeBody);
    REQUIRE(result.catalog.has_value());
    const Catalog& catalog = *result.catalog;

    CHECK(catalog.findItem("stick") == ItemId{0});
    CHECK(catalog.findItem("stone_axe") == ItemId{1});
    CHECK_FALSE(catalog.findItem("wood_log").has_value());
    CHECK_FALSE(catalog.findItem("").has_value());
    CHECK(catalog.item(ItemId{2}) == nullptr);
    CHECK(catalog.item(ItemId{0xFFFF}) == nullptr);
    CHECK(catalog.item(ItemId{0})->maxStack == 40);
    CHECK(catalog.item(ItemId{0})->model.empty());

    const RecipeDef* recipe = catalog.findRecipe("stone_axe");
    REQUIRE(recipe != nullptr);
    REQUIRE(recipe->inputs.size() == 1);
    CHECK(recipe->inputs[0] == ItemStack{ItemId{0}, 2});
    CHECK(recipe->output == ItemStack{ItemId{1}, 1});
    CHECK(recipe->ticks == 60);
    CHECK(catalog.recipesAt(Station::Hand).size() == 1);
    CHECK(catalog.recipesAt(Station::Forge).empty());
    CHECK(catalog.findRecipe("nope") == nullptr);
}

TEST_CASE("Catalogue : noms des catégories et des postes") {
    CHECK(parseItemCategory("equipment") == ItemCategory::Equipment);
    CHECK_FALSE(parseItemCategory("Tool").has_value());
    CHECK(itemCategoryName(ItemCategory::Ammo) == "ammo");
    CHECK(parseStation("workbench") == Station::Workbench);
    CHECK_FALSE(parseStation("").has_value());
    CHECK(stationName(Station::Forge) == "forge");
}

TEST_CASE("Catalogue : erreurs de syntaxe, avec le bon numéro de ligne") {
    CHECK(itemsError("\n\n[item stick\n").line == 3);
    CHECK(itemsError("[item]\n").line == 1);
    CHECK(itemsError("[item a b]\n").line == 1);
    CHECK(itemsError("# rien\nname = Bâton\n").line == 2);
    CHECK(itemsError("[item stick]\nname Bâton\n").line == 2);
    CHECK(itemsError("[item stick]\n = Bâton\n").line == 2);

    const DataError noValue = itemsError("[item stick]\nname = Bâton\nstack =   \n");
    CHECK(noValue.line == 3);
    CHECK(noValue.file == "items.txt");
    CHECK(noValue.message.find("stack") != std::string::npos);
    CHECK(noValue.describe() == "items.txt:3 : " + noValue.message);
}

TEST_CASE("Catalogue : clés inconnues, répétées ou manquantes") {
    const std::string base = "[item stick]\nname = Bâton\nstack = 4\ncategory = resource\n";
    CHECK(itemsError(base + "weight = 3\n").line == 5);
    CHECK(itemsError(base + "stack = 5\n").line == 5);
    CHECK(itemsError("\n[item stick]\nname = Bâton\nstack = 4\n").line == 2); // category absente
    CHECK(itemsError("[recipe stick]\n").line == 1);                          // mauvais type
    const std::string head(kRecipeHead);
    CHECK(loadError(kItems, head + "output = stone_axe 1\nticks = 60\n").line == 1); // sans input
    CHECK(loadError(kItems, head + kValidRecipeBody + "output = stick 1\n").line == 6);
}

TEST_CASE("Catalogue : identifiants invalides ou en double") {
    CHECK(itemsError("[item Stone]\n").line == 1);
    CHECK(itemsError("[item 1stone]\n").line == 1);
    CHECK(itemsError("[item hache-pierre]\n").line == 1);
    CHECK(itemsError("[item " + std::string(49, 'a') + "]\n").line == 1);

    const std::string stick = "[item stick]\nname = A\nstack = 1\ncategory = tool\n";
    CHECK(itemsError(stick + "\n" + stick).line == 6);

    const std::string recipe = std::string(kRecipeHead) + kValidRecipeBody;
    CHECK(loadError(kItems, recipe + recipe).line == 6);
}

TEST_CASE("Catalogue : valeurs d'objet hors bornes") {
    const auto item = [](std::string_view stack, std::string_view category,
                         std::string_view extra = "") {
        return "[item stick]\nname = A\nstack = " + std::string(stack) +
               "\ncategory = " + std::string(category) + "\n" + std::string(extra);
    };
    CHECK(itemsError(item("0", "tool")).line == 3);
    CHECK(itemsError(item("10000", "tool")).line == 3);
    CHECK(itemsError(item("-1", "tool")).line == 3);
    CHECK(itemsError(item("4x", "tool")).line == 3);
    CHECK(itemsError(item("99999999999999999999", "tool")).line == 3);
    CHECK(itemsError(item("4", "outil")).line == 4);
    CHECK(itemsError(item("4", "tool", "name2 = x\n")).line == 5);
    CHECK(itemsError(item("4", "tool", "model = ../secret\n")).line == 5);
    CHECK(itemsError(item("4", "tool", "model = outils//hache\n")).line == 5);
    CHECK(itemsError(item("4", "tool", "model = outils/hache.fbx\n")).line == 5);
    CHECK(itemsError(item("4", "tool", "model = /outils/hache\n")).line == 5);
    const std::string longName =
        "[item stick]\nname = " + std::string(97, 'a') + "\nstack = 1\ncategory = tool\n";
    CHECK(itemsError(longName).line == 2);
}

TEST_CASE("Catalogue : recettes invalides") {
    CHECK(loadError(kItems, "[recipe a]\nstation = anvil\ninput = stick 1\noutput = stick 1\n"
                            "ticks = 1\n")
              .line == 2);
    CHECK(recipeError("input = wood_log 1\n").line == 3);               // objet inconnu
    CHECK(recipeError("input = stick\n").line == 3);                    // quantité absente
    CHECK(recipeError("input = stick 0\n").line == 3);                  // quantité nulle
    CHECK(recipeError("input = stick 2 3\n").line == 3);                // trop de valeurs
    CHECK(recipeError("input = stick 2\ninput = stick 1\n").line == 4); // ingrédient répété
    CHECK(recipeError("input = stick 2\n", "gun 1").line == 4);         // résultat inconnu
    CHECK(recipeError("input = stick 2\n", "stone_axe 2").line == 4);   // plus que la pile max
    CHECK(recipeError("input = stick 2\n", "stone_axe 1", "0").line == 5);
    CHECK(recipeError("input = stick 2\n", "stone_axe 1", "12001").line == 5);
}

TEST_CASE("Catalogue : trop d'ingrédients") {
    std::string items;
    std::string body;
    for (int i = 0; i < 9; ++i) {
        const std::string key = "thing" + std::to_string(i);
        items += "[item " + key + "]\nname = X\nstack = 5\ncategory = resource\n";
        body += "input = " + key + " 1\n";
    }
    const DataError error =
        loadError(items, "[recipe x]\nstation = hand\n" + body + "output = thing0 1\nticks = 1\n");
    CHECK(error.line == 11);
}

TEST_CASE("Catalogue : texte hostile, sans plantage") {
    CHECK(itemsError("[item stick]\nname = " + std::string(5000, 'a') + "\n").line == 2);
    CHECK(itemsError(std::string("[item stick]\nna\0me = x\n", 23)).line == 2);
    CHECK(itemsError("[item stick]\nname = a\x01\n").line == 2);
    CHECK(itemsError("[]\n").line == 1);
    CHECK(itemsError("[\n").line == 1);
    CHECK(itemsError("=\n").line == 1);
    CHECK(itemsError("[item stick]\n=\n").line == 2);

    // Fichiers vides, BOM, fins de ligne Windows, pas de saut de ligne final : acceptés.
    CHECK(Catalog::fromText("", "").catalog.has_value());
    const CatalogResult windows = Catalog::fromText(
        "\xEF\xBB\xBF# objet\r\n[item stick]\r\nname = B\r\nstack = 2\r\ncategory = tool", "");
    REQUIRE(windows.catalog.has_value());
    CHECK(windows.catalog->item(ItemId{0})->name == "B");
}

TEST_CASE("Catalogue : fichier absent") {
    const CatalogResult result = Catalog::fromDirectory(kDataDir + "/absent");
    REQUIRE(result.error.has_value());
    CHECK(result.error->line == 0);
    CHECK(result.error->file == kDataDir + "/absent/items.txt");
}

TEST_CASE("Catalogue : octets aléatoires et fichiers abîmés") {
    std::mt19937_64 random(0xCA7A1065);
    const auto below = [&random](std::size_t bound) {
        return static_cast<std::size_t>(random() % bound);
    };
    const std::string recipes = std::string(kRecipeHead) + kValidRecipeBody;
    using namespace std::string_view_literals;
    const std::string_view alphabet = "[]= \n#\tabz019_/\r\0\xC3"sv;

    for (int i = 0; i < 5'000; ++i) {
        std::string items(kItems);
        const std::size_t changes = 1 + below(6);
        for (std::size_t c = 0; c < changes; ++c) {
            items[below(items.size())] = alphabet[below(alphabet.size())];
        }
        const CatalogResult result = Catalog::fromText(items, recipes);
        CHECK(result.catalog.has_value() != result.error.has_value());
        if (result.catalog) {
            for (const ItemDef& item : result.catalog->items()) {
                CHECK(item.maxStack >= 1);
            }
        }
    }
}
