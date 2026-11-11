#include "GameWorld.hpp"

#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using namespace odyssee;
using odyssee::server::GameWorld;

namespace {

net::InputBatch lookingAt(net::Tick tick, float yawDegrees) {
    net::InputCommand command;
    command.yaw = net::quantizeYaw(yawDegrees);
    return net::InputBatch{tick, {command}};
}

const net::EntityState* entityOf(const net::Snapshot& snapshot, net::EntityId id) {
    const auto it = std::find_if(snapshot.entities.begin(), snapshot.entities.end(),
                                 [id](const net::EntityState& e) { return e.id == id; });
    return it == snapshot.entities.end() ? nullptr : &*it;
}

} // namespace

TEST_CASE("GameWorld : les joueurs connectés apparaissent, ceux partis disparaissent") {
    GameWorld world;
    const std::vector<net::PlayerId> two{0, 2};
    world.syncPlayers(two);
    CHECK(world.playerCount() == 2);
    CHECK(world.hasPlayer(2));
    CHECK_FALSE(world.hasPlayer(1));

    const auto snapshot = world.snapshotFor(0, 50);
    CHECK(snapshot.serverTick == 50);
    CHECK(snapshot.entities.size() == 2);
    REQUIRE(entityOf(snapshot, 2) != nullptr);
    CHECK(entityOf(snapshot, 2)->kind == net::EntityKind::Player);

    const std::vector<net::PlayerId> one{2};
    world.syncPlayers(one);
    CHECK(world.playerCount() == 1);
    CHECK(world.snapshotFor(2, 51).entities.size() == 1);
}

TEST_CASE("GameWorld : deux joueurs n'apparaissent pas au même endroit") {
    GameWorld world;
    const std::vector<net::PlayerId> all{0, 1, 2, 3};
    world.syncPlayers(all);
    const auto snapshot = world.snapshotFor(0, 1);
    for (std::size_t i = 0; i < snapshot.entities.size(); ++i) {
        for (std::size_t j = i + 1; j < snapshot.entities.size(); ++j) {
            CHECK(snapshot.entities[i].position != snapshot.entities[j].position);
        }
    }
}

TEST_CASE("GameWorld : un input par tick ; l'orientation suit, le dernier input est signalé") {
    GameWorld world;
    const std::vector<net::PlayerId> one{1};
    world.syncPlayers(one);

    world.receiveInputs(1, lookingAt(500, 30.0f));
    world.receiveInputs(1, lookingAt(501, 60.0f));
    world.step();

    auto snapshot = world.snapshotFor(1, 10);
    CHECK(snapshot.lastProcessedInput == 500);
    CHECK(entityOf(snapshot, 1)->yaw == net::quantizeYaw(30.0f));

    world.step();
    snapshot = world.snapshotFor(1, 11);
    CHECK(snapshot.lastProcessedInput == 501);
    CHECK(entityOf(snapshot, 1)->yaw == net::quantizeYaw(60.0f));

    world.step(); // plus d'input : rien ne change
    CHECK(world.snapshotFor(1, 12).lastProcessedInput == 501);
}

TEST_CASE("GameWorld : les inputs d'un joueur absent sont ignorés") {
    GameWorld world;
    world.receiveInputs(3, lookingAt(1, 90.0f));
    world.step();
    CHECK(world.playerCount() == 0);
    CHECK(world.snapshotFor(3, 1).lastProcessedInput == 0);
}
