#include "GameWorld.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Movement.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using namespace odyssee;
using odyssee::server::ConnectedPlayer;
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
    const std::vector<ConnectedPlayer> two{{0, 1}, {2, 2}};
    world.syncPlayers(two);
    CHECK(world.playerCount() == 2);
    CHECK(world.hasPlayer(2));
    CHECK_FALSE(world.hasPlayer(1));

    const auto snapshot = world.snapshotFor(0, 50);
    CHECK(snapshot.serverTick == 50);
    CHECK(snapshot.entities.size() == 2);
    REQUIRE(entityOf(snapshot, 2) != nullptr);
    CHECK(entityOf(snapshot, 2)->kind == net::EntityKind::Player);

    const std::vector<ConnectedPlayer> one{{2, 2}};
    world.syncPlayers(one);
    CHECK(world.playerCount() == 1);
    CHECK(world.snapshotFor(2, 51).entities.size() == 1);
}

TEST_CASE("GameWorld : deux joueurs n'apparaissent pas au même endroit") {
    GameWorld world;
    const std::vector<ConnectedPlayer> all{{0, 1}, {1, 2}, {2, 3}, {3, 4}};
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
    const std::vector<ConnectedPlayer> one{{1, 1}};
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

TEST_CASE("GameWorld : le serveur et la prédiction du client arrivent au même état") {
    // Le serveur reçoit les inputs par lots redondants (comme sur le réseau) ; le client applique
    // sim::step lui-même, un input par tick, à partir de l'état d'apparition reçu.
    GameWorld world;
    const std::vector<ConnectedPlayer> one{{2, 1}};
    world.syncPlayers(one);
    sim::PlayerState predicted = *world.motionOf(2);
    const sim::FlatTerrain ground(predicted.position[2]); // le sol provisoire du serveur

    std::vector<net::InputCommand> sent;
    for (net::Tick tick = 1; tick <= 120; ++tick) {
        net::InputCommand command;
        command.moveForward = net::quantizeAxis(tick % 40 < 30 ? 1.0f : 0.0f);
        command.moveRight = net::quantizeAxis(tick % 25 < 5 ? -1.0f : 0.0f);
        command.yaw = net::quantizeYaw(static_cast<float>(tick) * 3.0f);
        command.buttons = static_cast<std::uint16_t>(
            (tick % 50 == 0 ? static_cast<unsigned>(net::InputButton::Jump) : 0U) |
            static_cast<unsigned>(net::InputButton::Sprint));
        sent.push_back(command);

        // Les 4 derniers inputs, comme le client les renvoie à chaque paquet.
        const std::size_t first = sent.size() > 4 ? sent.size() - 4 : 0;
        world.receiveInputs(2, net::InputBatch{static_cast<net::Tick>(first + 1),
                                               {sent.begin() + static_cast<std::ptrdiff_t>(first),
                                                sent.end()}});
        world.step();
        predicted = sim::step(predicted, command, ground);
    }

    REQUIRE(world.motionOf(2).has_value());
    CHECK(*world.motionOf(2) == predicted);
    CHECK(world.snapshotFor(2, 999).lastProcessedInput == 120);
}

TEST_CASE("GameWorld : une place reprise par une autre connexion repart de zéro") {
    GameWorld world;
    const std::vector<ConnectedPlayer> before{{1, 7}};
    world.syncPlayers(before);
    const sim::PlayerState spawn = *world.motionOf(1);

    // L'ancien joueur avance, et ses ticks d'input sont déjà loin.
    net::InputCommand forward;
    forward.moveForward = net::quantizeAxis(1.0f);
    forward.yaw = spawn.yaw;
    for (net::Tick tick = 5000; tick < 5020; ++tick) {
        world.receiveInputs(1, net::InputBatch{tick, {forward}});
        world.step();
    }
    REQUIRE(world.motionOf(1)->position != spawn.position);
    REQUIRE(world.snapshotFor(1, 1).lastProcessedInput == 5019);

    // Dans le même tick, il part et un autre client prend la place 1 : la place n'a jamais été
    // vue libre, seule la génération a changé.
    const std::vector<ConnectedPlayer> after{{1, 8}};
    world.syncPlayers(after);
    CHECK(*world.motionOf(1) == spawn);
    CHECK(world.snapshotFor(1, 2).lastProcessedInput == 0);

    // Le nouveau joueur commence ses ticks à 1 : ils ne sont pas pris pour de vieux inputs.
    world.receiveInputs(1, net::InputBatch{1, {forward}});
    world.step();
    CHECK(world.snapshotFor(1, 3).lastProcessedInput == 1);

    world.syncPlayers(after); // même génération : rien ne bouge
    CHECK(world.snapshotFor(1, 4).lastProcessedInput == 1);
}
