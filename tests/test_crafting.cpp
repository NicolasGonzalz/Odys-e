#include "Crafting.hpp"
#include "GameWorld.hpp"
#include "ItemUse.hpp"

#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <string_view>
#include <variant>
#include <vector>

using namespace odyssee;
using net::CraftOutcome;
using odyssee::server::ConnectedPlayer;
using odyssee::server::Crafting;
using odyssee::server::GameWorld;
using odyssee::server::Inventory;
using odyssee::server::Vitals;
using odyssee::server::VitalsTuning;

namespace {

constexpr std::string_view kItems = R"([item stick]
name = Bâton
stack = 40
category = resource

[item stone]
name = Pierre
stack = 40
category = resource

[item stone_axe]
name = Hache de pierre
stack = 1
category = tool

[item berries]
name = Baies
stack = 30
category = consumable
eat = 6
drink = 4
)";

constexpr std::string_view kRecipes = R"([recipe stone_axe]
station = hand
input = stick 1
input = stone 2
output = stone_axe 1
ticks = 10

[recipe bench_axe]
station = workbench
input = stick 1
output = stone_axe 1
ticks = 10
)";

data::Catalog catalog() {
    auto result = data::Catalog::fromText(kItems, kRecipes);
    REQUIRE(result.catalog);
    return std::move(*result.catalog);
}

const data::ItemId kStick{0}, kStone{1}, kAxe{2}, kBerries{3};

Inventory withMaterials(const data::Catalog& items) {
    Inventory inventory;
    (void)server::addItem(inventory, kStick, 3, items);
    (void)server::addItem(inventory, kStone, 5, items);
    return inventory;
}

} // namespace

TEST_CASE("Artisanat : les ingrédients sont pris au début, le résultat arrive à la fin") {
    const auto items = catalog();
    Inventory inventory = withMaterials(items);
    Crafting crafting;

    CHECK(server::startCraft(crafting, inventory, 0, items) == CraftOutcome::Started);
    CHECK(server::countItem(inventory, kStick) == 2);
    CHECK(server::countItem(inventory, kStone) == 3);
    CHECK(server::startCraft(crafting, inventory, 0, items) == CraftOutcome::Busy);

    for (int tick = 1; tick < 10; ++tick) {
        CHECK_FALSE(server::stepCraft(crafting, inventory, items));
    }
    CHECK(server::stepCraft(crafting, inventory, items) == std::uint16_t{0}); // 10e tick : finie
    CHECK(server::countItem(inventory, kAxe) == 1);
    CHECK_FALSE(server::stepCraft(crafting, inventory, items)); // plus rien en cours
}

TEST_CASE("Artisanat : refus sans ingrédients, sans poste, ou recette inconnue (rien ne change)") {
    const auto items = catalog();
    Crafting crafting;
    Inventory empty;
    CHECK(server::startCraft(crafting, empty, 0, items) == CraftOutcome::MissingIngredients);

    Inventory inventory = withMaterials(items);
    CHECK(server::startCraft(crafting, inventory, 1, items) == CraftOutcome::NeedStation);
    CHECK(server::startCraft(crafting, inventory, 9, items) == CraftOutcome::UnknownRecipe);
    CHECK(server::countItem(inventory, kStick) == 3);
    CHECK(crafting.ticksLeft == 0);
}

TEST_CASE("Objets : on mange ce qu'on tient, un exemplaire à la fois ; un outil ne se mange pas") {
    const auto items = catalog();
    Inventory inventory;
    (void)server::addItem(inventory, kBerries, 2, items); // case 0
    (void)server::addItem(inventory, kAxe, 1, items);     // case 1
    Vitals vitals{VitalsTuning::kVitalScale, 50'000, 90'000};

    CHECK(server::useItem(inventory, 0, vitals, items));
    CHECK(vitals.hunger == 56'000);
    CHECK(vitals.thirst == 94'000);
    CHECK(inventory.slots[0].count == 1);
    CHECK_FALSE(server::useItem(inventory, 1, vitals, items)); // hache
    CHECK_FALSE(server::useItem(inventory, 4, vitals, items)); // case vide

    Vitals full{};
    CHECK(server::useItem(inventory, 0, full, items));
    CHECK(full.hunger == VitalsTuning::kVitalScale); // jamais au-delà
    CHECK(inventory.slots[0] == net::InventorySlot{});
}

TEST_CASE("Objets : seul un outil compte comme outil tenu") {
    const auto items = catalog();
    Inventory inventory;
    (void)server::addItem(inventory, kAxe, 1, items);
    (void)server::addItem(inventory, kStick, 1, items);
    CHECK(server::heldToolOf(inventory, 0, items) == kAxe);
    CHECK_FALSE(server::heldToolOf(inventory, 1, items));
    CHECK_FALSE(server::heldToolOf(inventory, 3, items));
}

TEST_CASE("GameWorld : fabriquer à la main, et manger avec E (une fois par appui)") {
    GameWorld world;
    world.useCatalog(catalog());
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);
    CHECK(world.giveItem(0, kStick, 1) == 0);
    CHECK(world.giveItem(0, kStone, 2) == 0);
    CHECK(world.giveItem(0, kBerries, 3) == 0); // case 2

    world.receiveRequest(0, net::CraftRequest{0});
    auto events = world.takeEvents(0);
    REQUIRE(events.size() == 1);
    CHECK(std::get<net::CraftStatus>(events[0]) == net::CraftStatus{0, CraftOutcome::Started, 10});

    net::InputCommand eat;
    eat.heldSlot = 2;
    eat.buttons = static_cast<std::uint16_t>(net::InputButton::Use);
    for (net::Tick tick = 1; tick <= 10; ++tick) { // E tenu dix ticks : une seule baie mangée
        world.receiveInputs(0, net::InputBatch{tick, {eat}});
        world.step();
    }
    events = world.takeEvents(0);
    REQUIRE(events.size() == 1);
    CHECK(std::get<net::CraftStatus>(events[0]).outcome == CraftOutcome::Done);
    const Inventory& inventory = *world.inventoryOf(0);
    CHECK(server::countItem(inventory, kAxe) == 1);
    CHECK(server::countItem(inventory, kBerries) == 2);
}
