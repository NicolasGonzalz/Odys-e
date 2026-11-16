// Persistance du monde (T053) : la base SQLite, et ce que le monde y met et en reprend.
#include "GameWorld.hpp"
#include "persistence/SaveStore.hpp"
#include "persistence/Sqlite.hpp"

#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using namespace odyssee;
using odyssee::server::ConnectedPlayer;
using odyssee::server::GameWorld;
using odyssee::server::SavedBag;
using odyssee::server::SavedPlayer;
using odyssee::server::SaveStore;
using odyssee::server::Vitals;
using odyssee::server::WorldSave;

namespace {

constexpr std::string_view kItems = R"([item stone]
name = Pierre
stack = 40
category = resource
)";

/// Un rocher qui se récolte à la main : 2 coups.
constexpr std::string_view kResources = R"([resource rock]
name = Rocher
shape = rock
item = stone 2
hits = 2
hit_ticks = 5
regrow = 400
abundance = 1
)";

const data::ItemId kStone{0};

data::Catalog items() {
    return std::move(*data::Catalog::fromText(kItems, "").catalog);
}

/// Un fichier de base propre, effacé avant et après le test.
struct TempBase {
    std::string path;

    explicit TempBase(std::string_view name)
        : path((std::filesystem::temp_directory_path() /
                ("odyssee_test_" + std::string(name) + ".sqlite"))
                   .string()) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
    /// Sans exception : sous Windows, un fichier encore ouvert ne s'efface pas.
    ~TempBase() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

/// Un monde sans carte, avec un rocher 1,5 m au nord du joueur 0 (comme au démarrage du
/// serveur : ressources placées avant l'arrivée des joueurs, toujours aux mêmes places).
GameWorld freshWorld() {
    GameWorld world;
    world.useCatalog(items());
    auto types = data::ResourceCatalog::fromText(kResources, world.catalog());
    REQUIRE(types.catalog);
    // Pieds du joueur 0 : le point d'apparition, décalé de 3 m vers l'ouest (GameWorld::spawn).
    GameWorld probe;
    const std::vector<ConnectedPlayer> one{{0, 1}};
    probe.syncPlayers(one);
    auto rockAt = probe.motionOf(0)->position;
    rockAt[1] -= 384;
    const std::vector<server::PlacedResource> rock{{data::ResourceTypeId{0}, rockAt, 0}};
    CHECK(world.addResources(std::move(*types.catalog), rock) == 1);
    return world;
}

void stepHolding(GameWorld& world, net::PlayerId player, net::Tick tick, std::uint16_t buttons,
                 std::int8_t forward = 0) {
    net::InputCommand command;
    command.yaw = net::quantizeYaw(270.0f); // vers le nord : face au rocher
    command.buttons = buttons;
    command.moveForward = forward;
    world.receiveInputs(player, net::InputBatch{tick, {command}});
    world.step();
}

WorldSave sampleSave() {
    WorldSave save;
    save.worldFingerprint = 0x1234'5678'9ABC'DEF0;
    SavedPlayer mara;
    mara.name = "Mara";
    mara.position = {1000, -2000, 512};
    mara.yaw = 16384;
    mara.vitals = Vitals{70'000, 40'000, 20'000};
    mara.inventory[0] = {3, 12};
    mara.inventory[24] = {7, 1};
    mara.rest = {{10, 20, 30}, 5};
    SavedPlayer teo = mara;
    teo.name = "Téo"; // accents : le nom est en UTF-8
    teo.inventory = {};
    save.players = {mara, teo};
    save.resources = {{0, 2, 0}, {1, 0, 380}};
    SavedBag bag;
    bag.position = {7, 8, 9};
    bag.contents.owner = 2;
    bag.contents.slots[4] = {1, 30};
    save.bags = {bag};
    return save;
}

} // namespace

TEST_CASE("Sauvegarde : une base neuve est vide, puis rend exactement ce qu'on y a mis") {
    TempBase base("aller_retour");
    {
        auto store = SaveStore::open(base.path);
        REQUIRE(store);
        const auto empty = store->load();
        REQUIRE(empty);
        CHECK(*empty == WorldSave{});
        CHECK(store->store(sampleSave()));
    }
    auto reopened = SaveStore::open(base.path); // comme après un redémarrage
    REQUIRE(reopened);
    CHECK(reopened->load() == sampleSave());
}

TEST_CASE("Sauvegarde : chaque écriture remplace la précédente") {
    TempBase base("remplace");
    auto store = SaveStore::open(base.path);
    REQUIRE(store);
    CHECK(store->store(sampleSave()));
    WorldSave smaller = sampleSave();
    smaller.players.pop_back();
    smaller.bags.clear();
    CHECK(store->store(smaller));
    CHECK(store->load() == smaller);
}

TEST_CASE("Sauvegarde : une base d'une version plus récente du serveur est refusée") {
    TempBase base("trop_recente");
    {
        auto store = SaveStore::open(base.path);
        REQUIRE(store);
        CHECK(store->store(sampleSave()));
    }
    {
        auto db = server::sqlite::Database::open(base.path);
        REQUIRE(db);
        CHECK(db->exec("UPDATE meta SET value = 99 WHERE key = 'schema_version'"));
    }
    CHECK_FALSE(SaveStore::open(base.path));
}

TEST_CASE("Monde : un joueur qui se reconnecte sous le même nom retrouve tout") {
    GameWorld world = freshWorld();
    world.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Mara"}});
    CHECK(world.giveItem(0, kStone, 11) == 0);
    for (net::Tick tick = 1; tick <= 20; ++tick) {
        stepHolding(world, 0, tick, 0, 127); // elle marche un peu vers le nord
    }
    world.setVitals(0, Vitals{60'000, 50'000, 40'000});
    const auto before = *world.motionOf(0);
    const auto inventory = world.inventoryOf(0)->slots;

    world.syncPlayers({});                                           // elle part…
    world.syncPlayers(std::vector<ConnectedPlayer>{{2, 7, "Mara"}}); // …et revient, autre place
    CHECK(world.motionOf(2)->position == before.position);
    CHECK(world.inventoryOf(2)->slots == inventory);
    CHECK(world.vitalsOf(2) == Vitals{60'000, 50'000, 40'000});
}

TEST_CASE("Monde : sauvegardé puis repris dans un monde neuf (joueurs, ressources, sacs)") {
    GameWorld before = freshWorld();
    before.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Mara"}, {1, 2, "Ilian"}});
    CHECK(before.giveItem(0, kStone, 7) == 0);
    const auto primary = static_cast<std::uint16_t>(net::InputButton::Primary);
    stepHolding(before, 0, 1, primary); // Mara frappe le rocher une fois
    REQUIRE(before.resourceNode(net::kFirstResourceEntity)->hitsLeft == 1);
    CHECK(before.giveItem(1, kStone, 3) == 0);
    for (net::Tick tick = 1; tick <= 20; ++tick) {
        stepHolding(before, 1, tick, 0, 127); // Ilian s'éloigne de son point de repos…
    }
    before.setVitals(1, Vitals{0, 0, 0});
    before.step(); // …et meurt : son sac reste là où il est tombé
    REQUIRE(before.bags().size() == 1);

    TempBase base("monde");
    {
        auto store = SaveStore::open(base.path);
        REQUIRE(store);
        CHECK(store->store(before.save()));
    }
    auto store = SaveStore::open(base.path);
    REQUIRE(store);
    const auto save = store->load();
    REQUIRE(save);
    CHECK(*save == before.save());

    GameWorld after = freshWorld(); // le serveur redémarré
    after.load(*save);
    after.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Mara"}});
    CHECK(after.motionOf(0)->position == before.motionOf(0)->position);
    CHECK(after.inventoryOf(0)->slots == before.inventoryOf(0)->slots);
    CHECK(after.vitalsOf(0) == before.vitalsOf(0));
    CHECK(after.resourceNode(net::kFirstResourceEntity)->hitsLeft == 1);
    REQUIRE(after.bags().size() == 1);
    CHECK(after.save().bags == before.save().bags);
    CHECK(after.save().players == before.save().players); // Ilian, absent, est toujours là
}

TEST_CASE("Monde : deux joueurs du même nom ; le second repart de zéro et n'est pas gardé") {
    GameWorld world = freshWorld();
    world.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Nadia"}});
    CHECK(world.giveItem(0, kStone, 5) == 0);
    world.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Nadia"}, {1, 2, "Nadia"}});
    CHECK(server::countItem(*world.inventoryOf(1), kStone) == 0);
    const WorldSave save = world.save();
    REQUIRE(save.players.size() == 1);
    CHECK(save.players[0].inventory[0] == net::InventorySlot{kStone.value, 5});
}

TEST_CASE("Monde : sauvegarde d'une autre carte ; inventaire gardé, position remise à zéro") {
    GameWorld world = freshWorld();
    WorldSave old = sampleSave(); // empreinte de carte différente (le monde sans carte : 0)
    old.players[0].inventory = {};
    old.players[0].inventory[0] = {kStone.value, 4};
    world.load(old);
    world.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Mara"}});

    GameWorld reference = freshWorld();
    reference.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Anonyme"}});
    CHECK(world.motionOf(0)->position[1] == reference.motionOf(0)->position[1]);
    CHECK(server::countItem(*world.inventoryOf(0), kStone) == 4);
    CHECK(world.resourceNode(net::kFirstResourceEntity)->hitsLeft == 2); // pas l'ancien état
    CHECK(world.bags().size() == 0);
}

TEST_CASE("Sauvegarde : une base au format 1 est mise à jour ; joueurs gardés, ressources oubliées") {
    TempBase base("format_1");
    {
        auto db = server::sqlite::Database::open(base.path);
        REQUIRE(db);
        CHECK(db->exec(R"(
CREATE TABLE meta(key TEXT PRIMARY KEY, value INTEGER NOT NULL);
CREATE TABLE players(
    name TEXT PRIMARY KEY, x INTEGER NOT NULL, y INTEGER NOT NULL, z INTEGER NOT NULL,
    yaw INTEGER NOT NULL, health INTEGER NOT NULL, hunger INTEGER NOT NULL,
    thirst INTEGER NOT NULL, rest_x INTEGER NOT NULL, rest_y INTEGER NOT NULL,
    rest_z INTEGER NOT NULL, rest_yaw INTEGER NOT NULL, inventory BLOB NOT NULL);
CREATE TABLE resources(rank INTEGER PRIMARY KEY, hits INTEGER NOT NULL, regrow INTEGER NOT NULL);
INSERT INTO meta VALUES ('schema_version', 1);
INSERT INTO players VALUES ('Mara', 1, 2, 3, 0, 100000, 100000, 100000, 1, 2, 3, 0, x'');
INSERT INTO resources VALUES (0, 1, 50);
)"));
    }
    auto store = SaveStore::open(base.path);
    REQUIRE(store);
    const auto save = store->load();
    REQUIRE(save);
    REQUIRE(save->players.size() == 1);
    CHECK(save->players[0].name == "Mara");
    CHECK(save->resources.empty());
    CHECK(store->store(sampleSave())); // et elle s'écrit au nouveau format
    CHECK(store->load() == sampleSave());
}

TEST_CASE("Monde : ressources sauvegardées d'autres types (données changées) ; elles repartent pleines") {
    GameWorld world = freshWorld();
    WorldSave save;
    save.players.push_back(SavedPlayer{});
    save.players[0].name = "Mara";
    save.resources = {{7, 1, 100}}; // même nombre, mais le type 7 n'est pas un rocher
    world.load(save);
    CHECK(world.resourceNode(net::kFirstResourceEntity)->hitsLeft == 2);

    save.resources = {{0, 1, 0}}; // le bon type : l'état est repris
    GameWorld same = freshWorld();
    same.load(save);
    CHECK(same.resourceNode(net::kFirstResourceEntity)->hitsLeft == 1);
}
