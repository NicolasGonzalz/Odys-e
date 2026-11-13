#include "GameWorld.hpp"
#include "Inventory.hpp"

#include <doctest/doctest.h>

#include <string_view>
#include <variant>
#include <vector>

using namespace odyssee;
using odyssee::server::ConnectedPlayer;
using odyssee::server::GameWorld;
using odyssee::server::Inventory;

namespace {

constexpr std::string_view kItems = R"([item stick]
name = Bâton
stack = 40
category = resource

[item stone_axe]
name = Hache de pierre
stack = 1
category = tool
)";

data::Catalog testCatalog() {
    auto result = data::Catalog::fromText(kItems, "");
    REQUIRE(result.catalog);
    return std::move(*result.catalog);
}

const data::ItemId kStick{0};
const data::ItemId kAxe{1};

} // namespace

TEST_CASE("Inventaire : les objets s'empilent jusqu'à la pile max, puis dans les cases vides") {
    const auto catalog = testCatalog();
    Inventory inventory;
    CHECK(server::addItem(inventory, kStick, 30, catalog) == 0);
    CHECK(server::addItem(inventory, kStick, 30, catalog) == 0);
    CHECK(inventory.slots[0].count == 40); // pile pleine
    CHECK(inventory.slots[1].count == 20);
    CHECK(server::countItem(inventory, kStick) == 60);

    CHECK(server::addItem(inventory, kAxe, 2, catalog) == 0); // pile de 1 : deux cases
    CHECK(inventory.slots[2].count == 1);
    CHECK(inventory.slots[3].count == 1);
}

TEST_CASE("Inventaire : plein, le reste est rendu ; objet inconnu refusé") {
    const auto catalog = testCatalog();
    Inventory inventory;
    CHECK(server::addItem(inventory, kAxe, 30, catalog) == 30 - net::kInventorySlots);
    CHECK(server::addItem(inventory, kStick, 5, catalog) == 5);
    Inventory empty;
    CHECK(server::addItem(empty, data::ItemId{99}, 3, catalog) == 3);
}

TEST_CASE("Inventaire : retirer seulement s'il y en a assez") {
    const auto catalog = testCatalog();
    Inventory inventory;
    (void)server::addItem(inventory, kStick, 50, catalog);
    CHECK_FALSE(server::removeItem(inventory, kStick, 51));
    CHECK(server::countItem(inventory, kStick) == 50);
    CHECK(server::removeItem(inventory, kStick, 45));
    CHECK(server::countItem(inventory, kStick) == 5);
    CHECK(inventory.slots[1] == net::InventorySlot{}); // vidée : case vide propre
}

TEST_CASE("Inventaire : déplacer échange deux cases, ou regroupe une même pile") {
    const auto catalog = testCatalog();
    Inventory inventory;
    (void)server::addItem(inventory, kAxe, 1, catalog);    // case 0
    (void)server::addItem(inventory, kStick, 30, catalog); // case 1
    CHECK(server::moveItem(inventory, 0, 1, catalog));     // échange
    CHECK(inventory.slots[0].item == kStick.value);
    CHECK(inventory.slots[1].item == kAxe.value);

    inventory.slots[5] = {kStick.value, 25};
    CHECK(server::moveItem(inventory, 5, 0, catalog)); // 30 + 25 : pile pleine à 40, reste 15
    CHECK(inventory.slots[0].count == 40);
    CHECK(inventory.slots[5].count == 15);

    CHECK_FALSE(server::moveItem(inventory, 7, 0, catalog));  // case vide
    CHECK_FALSE(server::moveItem(inventory, 0, 99, catalog)); // hors limites
    CHECK_FALSE(server::moveItem(inventory, 0, 0, catalog));
}

TEST_CASE("GameWorld : l'inventaire est envoyé à l'apparition, puis seulement s'il change") {
    GameWorld world;
    world.useCatalog(testCatalog());
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);

    CHECK(world.takeInventoryUpdate(0)); // inventaire vide, envoyé une fois
    CHECK_FALSE(world.takeInventoryUpdate(0));

    CHECK(world.giveItem(0, kStick, 3) == 0);
    const auto update = world.takeInventoryUpdate(0);
    REQUIRE(update);
    CHECK(update->slots[0] == net::InventorySlot{kStick.value, 3});

    world.receiveRequest(0, net::MoveItem{0, 4});
    const auto moved = world.takeInventoryUpdate(0);
    REQUIRE(moved);
    CHECK(moved->slots[4] == net::InventorySlot{kStick.value, 3});

    world.receiveRequest(0, net::MoveItem{0, 1}); // case vide : refusé, état réel renvoyé
    CHECK(world.takeInventoryUpdate(0));
    CHECK(world.giveItem(3, kStick, 1) == 1); // joueur absent
}
