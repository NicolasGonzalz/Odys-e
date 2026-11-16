// Exploration et carte commune (T074).
#include "GameWorld.hpp"
#include "exploration/Explorer.hpp"
#include "exploration/WorldMap.hpp"
#include "persistence/SaveStore.hpp"

#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <variant>
#include <vector>

using namespace odyssee;
using odyssee::server::ConnectedPlayer;
using odyssee::server::Explorer;
using odyssee::server::GameWorld;
using odyssee::server::WorldMap;

namespace {

constexpr std::int32_t kChunk = net::kMapChunkMeters * 256;

/// Le joueur avance vers `yawDegrees` pendant `ticks` ticks (inputs à partir de `firstTick`).
void walk(GameWorld& world, net::PlayerId player, net::Tick firstTick, int ticks,
          float yawDegrees) {
    net::InputCommand forward;
    forward.moveForward = 127;
    forward.yaw = net::quantizeYaw(yawDegrees);
    for (int i = 0; i < ticks; ++i) {
        world.receiveInputs(player,
                            net::InputBatch{firstTick + static_cast<net::Tick>(i), {forward}});
        world.step();
    }
}

std::size_t sharedChunksReceived(GameWorld& world, net::PlayerId player) {
    std::size_t count = 0;
    for (const auto& event : world.takeEvents(player)) {
        const auto* map = std::get_if<net::MapChunks>(&event);
        if (map && map->layer == net::MapLayer::Shared) {
            count += map->chunks.size();
        }
    }
    return count;
}

} // namespace

TEST_CASE("Carte : carrés de 64 m, y compris sous zéro") {
    CHECK(server::chunkOf({0, 0, 0}) == net::MapChunk{0, 0});
    CHECK(server::chunkOf({kChunk - 1, kChunk, 0}) == net::MapChunk{0, 1});
    CHECK(server::chunkOf({-1, -kChunk, 0}) == net::MapChunk{-1, -1});
    CHECK(server::chunkOf({-kChunk - 1, 0, 0}) == net::MapChunk{-2, 0});
    CHECK(server::chunksAround({5, 5}, 1).size() == 9);
}

TEST_CASE("Carte : envoyée par lots, le premier vide la couche") {
    std::vector<net::MapChunk> many(450);
    for (std::size_t i = 0; i < many.size(); ++i) {
        many[i] = {static_cast<std::int16_t>(i), 0};
    }
    const auto events = server::toEvents(net::MapLayer::Shared, true, many);
    REQUIRE(events.size() == 3);
    CHECK(events[0].reset);
    CHECK_FALSE(events[1].reset);
    CHECK(events[2].chunks.size() == 50);

    const auto clear = server::toEvents(net::MapLayer::Pending, true, {});
    REQUIRE(clear.size() == 1);
    CHECK(clear[0].reset);
    CHECK(clear[0].chunks.empty());
    CHECK(server::toEvents(net::MapLayer::Pending, false, {}).empty());
}

TEST_CASE("Carte : en marchant, on mémorise ce qui n'est pas déjà sur la carte commune") {
    WorldMap map;
    (void)map.reveal(server::chunksAround({0, 0}, 0));
    Explorer explorer;
    CHECK(server::noteSurroundings(explorer, map, {0, 0}).size() == 8); // son carré est connu
    CHECK(server::noteSurroundings(explorer, map, {0, 0}).empty());     // même carré : rien
    CHECK(server::noteSurroundings(explorer, map, {1, 0}).size() == 3); // une colonne de neuf
}

TEST_CASE("Monde : zone de départ connue ; on explore, puis on reporte à la table pour tous") {
    GameWorld world;
    world.addShelterMapTable();
    CHECK(world.worldMap().chunks().size() == 25); // 5 × 5 autour du point d'apparition
    // Joueur 3 : il apparaît à 2 m de la table de l'abri ; joueur 0 regarde.
    world.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Ana"}, {3, 2, "Teo"}});
    CHECK(sharedChunksReceived(world, 0) == 25); // la carte commune, à l'apparition
    (void)world.takeEvents(3);

    walk(world, 3, 1, 1500, 0.0f); // ~300 m vers l'est : hors de la zone de départ
    const auto remembered = world.pendingMapOf(3);
    REQUIRE(remembered);
    CHECK(!remembered->empty());
    CHECK(world.worldMap().chunks().size() == 25); // rien de reporté en route

    walk(world, 3, 1501, 1500, 180.0f); // retour à l'abri, devant la table
    CHECK(world.pendingMapOf(3)->empty());
    CHECK(world.worldMap().chunks().size() == 25 + remembered->size());
    CHECK(sharedChunksReceived(world, 0) == remembered->size()); // Ana voit la carte grandir
}

TEST_CASE("Monde : carte commune et souvenirs survivent à une sauvegarde") {
    GameWorld before;
    before.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Ana"}});
    walk(before, 0, 1, 1500, 0.0f);
    const auto remembered = *before.pendingMapOf(0);
    REQUIRE(!remembered.empty());

    const auto path =
        (std::filesystem::temp_directory_path() / "odyssee_test_carte.sqlite").string();
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::optional<server::WorldSave> save;
    {
        auto store = server::SaveStore::open(path);
        REQUIRE(store);
        REQUIRE(store->store(before.save()));
        save = store->load();
    }
    std::filesystem::remove(path, ignored);
    REQUIRE(save);
    CHECK(*save == before.save());

    GameWorld after;
    after.load(*save);
    after.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Ana"}});
    CHECK(after.pendingMapOf(0) == remembered);
    CHECK(after.worldMap().chunks() == before.worldMap().chunks());
}
