#include "GameWorld.hpp"
#include "death/Death.hpp"
#include "death/DroppedBags.hpp"

#include "odyssee/net/GameEvents.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <string_view>
#include <variant>
#include <vector>

using namespace odyssee;
using odyssee::server::BagContents;
using odyssee::server::ConnectedPlayer;
using odyssee::server::DroppedBags;
using odyssee::server::GameWorld;
using odyssee::server::Inventory;
using odyssee::server::Vitals;
using odyssee::server::VitalsTuning;

namespace {

constexpr std::string_view kItems = R"([item stone]
name = Pierre
stack = 40
category = resource

[item stone_axe]
name = Hache de pierre
stack = 1
category = tool

[item map]
name = Carte
stack = 1
category = equipment
)";

data::Catalog catalog() {
    auto result = data::Catalog::fromText(kItems, "");
    REQUIRE(result.catalog);
    return std::move(*result.catalog);
}

const data::ItemId kStone{0}, kAxe{1}, kMap{2};

/// Un monde à un joueur, qui porte 30 pierres, une hache et la carte.
GameWorld worldWithLoadedPlayer() {
    GameWorld world;
    world.useCatalog(catalog());
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);
    CHECK(world.giveItem(0, kStone, 30) == 0);
    CHECK(world.giveItem(0, kAxe, 1) == 0);
    CHECK(world.giveItem(0, kMap, 1) == 0);
    return world;
}

void kill(GameWorld& world, net::PlayerId player) {
    world.setVitals(player, Vitals{0, 0, 0});
    world.step();
}

/// Le joueur avance vers l'est pendant `ticks` ticks (inputs à partir de `firstTick`).
void walkEast(GameWorld& world, net::PlayerId player, net::Tick firstTick, int ticks) {
    net::InputCommand forward;
    forward.moveForward = 127;
    forward.yaw = 0; // vers l'est
    for (int i = 0; i < ticks; ++i) {
        world.receiveInputs(player,
                            net::InputBatch{firstTick + static_cast<net::Tick>(i), {forward}});
        world.step();
    }
}

} // namespace

TEST_CASE("Mort : l'inventaire tombe, l'équipement reste") {
    const auto items = catalog();
    Inventory inventory;
    (void)server::addItem(inventory, kStone, 12, items);
    (void)server::addItem(inventory, kMap, 1, items);
    inventory.changed = false;

    const auto dropped = server::takeDroppedItems(inventory, items);
    CHECK(server::countItem(inventory, kStone) == 0);
    CHECK(server::countItem(inventory, kMap) == 1);
    CHECK(inventory.changed);
    CHECK(std::count(dropped.begin(), dropped.end(), net::InventorySlot{kStone.value, 12}) == 1);

    const Vitals back = server::respawnVitals();
    CHECK(back.health == VitalsTuning::kVitalScale);
    CHECK(back.hunger == VitalsTuning::kVitalScale / 2);
}

TEST_CASE("GameWorld : à 0 de vie, sac sur place et retour au point de repos") {
    GameWorld world = worldWithLoadedPlayer();
    const auto start = *world.motionOf(0);
    walkEast(world, 0, 1, 40); // 2 s de marche : il meurt loin de son point de repos
    const auto deathPlace = world.motionOf(0)->position;
    REQUIRE(deathPlace[0] > start.position[0] + 256);
    (void)world.takeEvents(0);

    kill(world, 0);

    const auto events = world.takeEvents(0);
    REQUIRE(events.size() == 1);
    const auto died = std::get<net::PlayerDied>(events[0]);
    CHECK(died.bag == net::kFirstBagEntity);
    const auto bag = world.bagContents(died.bag);
    REQUIRE(bag);
    CHECK(bag->owner == 0);
    CHECK(std::count(bag->slots.begin(), bag->slots.end(), net::InventorySlot{kStone.value, 30}) ==
          1);

    const Inventory& inventory = *world.inventoryOf(0);
    CHECK(server::countItem(inventory, kStone) == 0);
    CHECK(server::countItem(inventory, kAxe) == 0);
    CHECK(server::countItem(inventory, kMap) == 1); // équipement gardé
    CHECK(world.motionOf(0)->position == start.position);
    CHECK(world.vitalsOf(0)->health == VitalsTuning::kVitalScale);

    // Le sac est dans le snapshot, à l'endroit de la mort (le tick de la mort fait encore un pas,
    // moins de 30 cm).
    const auto snapshot = world.snapshotFor(0, 1);
    const auto seen =
        std::find_if(snapshot.entities.begin(), snapshot.entities.end(),
                     [](const net::EntityState& e) { return e.kind == net::EntityKind::Bag; });
    REQUIRE(seen != snapshot.entities.end());
    CHECK(seen->position[0] >= deathPlace[0]);
    CHECK(seen->position[0] - deathPlace[0] < 77);
    CHECK(seen->position[1] == deathPlace[1]);
}

TEST_CASE("GameWorld : on ramasse son sac en repassant dessus ; vide, il disparaît") {
    GameWorld world = worldWithLoadedPlayer();
    walkEast(world, 0, 1, 40);
    kill(world, 0);
    const auto events = world.takeEvents(0);
    const auto death = std::find_if(events.begin(), events.end(), [](const net::GameEvent& e) {
        return std::holds_alternative<net::PlayerDied>(e);
    });
    REQUIRE(death != events.end());
    const auto died = std::get<net::PlayerDied>(*death);
    REQUIRE(world.bags().size() == 1);

    walkEast(world, 0, 41, 39); // même chemin : il repasse sur le sac
    CHECK(server::countItem(*world.inventoryOf(0), kStone) == 30);
    CHECK(server::countItem(*world.inventoryOf(0), kAxe) == 1);
    CHECK(world.bags().size() == 0);
    CHECK_FALSE(world.bagContents(died.bag));
}

TEST_CASE("GameWorld : mourir les mains vides ne laisse pas de sac") {
    GameWorld world;
    world.useCatalog(catalog());
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);
    (void)world.takeEvents(0); // à l'apparition : la carte (T074)
    kill(world, 0);
    const auto events = world.takeEvents(0);
    REQUIRE(events.size() == 1);
    CHECK(std::get<net::PlayerDied>(events[0]).bag == 0);
    CHECK(world.bags().size() == 0);
}

TEST_CASE("Sacs : un inventaire plein ne prend que ce qui tient, le reste attend") {
    const auto items = catalog();
    server::ecs::Registry registry;
    DroppedBags bags;
    BagContents contents;
    contents.slots[0] = {kAxe.value, 1};
    contents.slots[1] = {kStone.value, 5};
    const auto id = bags.drop(registry, {0, 0, 0}, contents);
    REQUIRE(id);

    Inventory full;
    for (auto& slot : full.slots) {
        slot = {kStone.value, 39}; // encore une pierre par case, mais aucune case vide
    }
    CHECK(bags.pickUp(registry, {100, 0, 0}, full, items));
    const auto left = bags.contentsOf(registry, *id);
    REQUIRE(left);
    CHECK(left->slots[0] == net::InventorySlot{kAxe.value, 1}); // pas de case pour la hache
    CHECK(left->slots[1].count < 5);

    CHECK_FALSE(bags.pickUp(registry, {DroppedBags::kPickupRadius + 1, 0, 0}, full, items));
}
