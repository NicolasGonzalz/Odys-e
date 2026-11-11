#include "GameWorld.hpp"

#include "odyssee/client/Prediction.hpp"
#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/ProvisionalGround.hpp"
#include "odyssee/sim/Movement.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
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

    // Plus d'input : les ticks manquants sont devinés (dernier input rejoué) et comptés comme
    // traités, mais pas indéfiniment.
    world.step();
    snapshot = world.snapshotFor(1, 12);
    CHECK(snapshot.lastProcessedInput == 502);
    CHECK(entityOf(snapshot, 1)->yaw == net::quantizeYaw(60.0f));

    for (int i = 0; i < 20; ++i) {
        world.step();
    }
    snapshot = world.snapshotFor(1, 40);
    CHECK(snapshot.lastProcessedInput == 501 + GameWorld::kMaxGuessedInputs);
    CHECK(entityOf(snapshot, 1)->yaw == net::quantizeYaw(60.0f)); // l'orientation est gardée
}

TEST_CASE("GameWorld : un input deviné puis arrivé en retard n'est pas rejoué") {
    GameWorld world;
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);

    world.receiveInputs(0, lookingAt(10, 30.0f));
    world.step(); // 10 appliqué
    world.step(); // 11 manque : deviné
    REQUIRE(world.snapshotFor(0, 1).lastProcessedInput == 11);

    // Le lot en retard répète 10 et 11, et apporte 12 : seul 12 est nouveau.
    net::InputBatch late = lookingAt(10, 30.0f);
    late.commands.push_back(late.commands.front());
    late.commands.push_back(lookingAt(12, 90.0f).commands.front());
    world.receiveInputs(0, late);
    world.step();
    const auto snapshot = world.snapshotFor(0, 2);
    CHECK(snapshot.lastProcessedInput == 12);
    CHECK(entityOf(snapshot, 0)->yaw == net::quantizeYaw(90.0f));
}

TEST_CASE("GameWorld : sans input, le joueur est quand même simulé (pas de suspension en l'air)") {
    GameWorld world;
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);
    const sim::PlayerState spawn = *world.motionOf(0);

    // Avant tout input : le joueur reste posé, tourné comme à l'apparition.
    world.step();
    CHECK(*world.motionOf(0) == spawn);
    CHECK(world.snapshotFor(0, 1).lastProcessedInput == 0);

    // Il saute en avançant, puis son client se tait (client modifié, ou connexion coupée).
    net::InputCommand jump;
    jump.yaw = spawn.yaw;
    jump.moveForward = net::quantizeAxis(1.0f);
    jump.buttons = static_cast<std::uint16_t>(net::InputButton::Jump);
    world.receiveInputs(0, net::InputBatch{1, {jump}});
    world.step();
    REQUIRE(world.motionOf(0)->position[2] > spawn.position[2]);
    REQUIRE_FALSE(world.motionOf(0)->has(sim::kGrounded));

    for (int i = 0; i < 60; ++i) { // 3 secondes de silence
        world.step();
    }
    const sim::PlayerState after = *world.motionOf(0);
    CHECK(after.position[2] == spawn.position[2]); // retombé
    CHECK(after.has(sim::kGrounded));
    CHECK(after.velocity == std::array<std::int16_t, 3>{}); // et arrêté : plus d'input rejoué

    // Ses ticks n'ont avancé que des quelques inputs devinés : il peut reprendre.
    const net::Tick resume = world.snapshotFor(0, 99).lastProcessedInput + 1;
    CHECK(resume == 2 + GameWorld::kMaxGuessedInputs);
    world.receiveInputs(0, net::InputBatch{resume, {jump}});
    world.step();
    CHECK(world.snapshotFor(0, 100).lastProcessedInput == resume);
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

TEST_CASE("Prédiction du client : démarrée sur le snapshot du serveur, elle arrive au même état") {
    // Le vrai chemin du client (T031) : client::Prediction démarre sur le snapshot que le serveur
    // lui envoie, joue chaque input tout de suite, et envoie ses lots redondants au serveur.
    GameWorld world;
    const std::vector<ConnectedPlayer> one{{2, 1}};
    world.syncPlayers(one);

    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    client::Prediction prediction(ground);
    REQUIRE(prediction.start(world.snapshotFor(2, 1), 2));

    for (net::Tick tick = 1; tick <= 150; ++tick) {
        net::InputCommand command;
        command.moveForward = net::quantizeAxis(tick % 40 < 30 ? 1.0f : -0.5f);
        command.moveRight = net::quantizeAxis(tick % 25 < 5 ? 1.0f : 0.0f);
        command.yaw = net::quantizeYaw(static_cast<float>(tick) * 2.5f);
        command.buttons = static_cast<std::uint16_t>(
            (tick % 45 == 0 ? static_cast<unsigned>(net::InputButton::Jump) : 0U) |
            (tick % 60 < 30 ? static_cast<unsigned>(net::InputButton::Sprint) : 0U));

        REQUIRE(prediction.advance(command));
        world.receiveInputs(2, prediction.history().recentBatch());
        world.step();
    }

    REQUIRE(world.motionOf(2).has_value());
    CHECK(*world.motionOf(2) == *prediction.state());
    CHECK(world.snapshotFor(2, 151).lastProcessedInput == 150);
}

namespace {

/// Un input qui change souvent (direction, cap, course, sauts) : un input deviné en répétant le
/// précédent est donc souvent faux.
net::InputCommand varied(net::Tick tick) {
    net::InputCommand command;
    command.moveForward = net::quantizeAxis(tick % 20 < 12 ? 1.0f : -1.0f);
    command.moveRight = net::quantizeAxis(tick % 7 < 3 ? 1.0f : -1.0f);
    command.yaw = net::quantizeYaw(static_cast<float>(tick % 36) * 10.0f);
    command.buttons = static_cast<std::uint16_t>(
        (tick % 30 == 0 ? static_cast<unsigned>(net::InputButton::Jump) : 0U) |
        (tick % 16 < 8 ? static_cast<unsigned>(net::InputButton::Sprint) : 0U));
    return command;
}

/// Joue une partie : le client prédit, envoie ses lots (sauf pendant les ticks `lost`), et se
/// réconcilie avec des snapshots qui arrivent `delay` ticks en retard. Puis il s'arrête et laisse
/// le serveur tout rattraper.
struct ReconciledGame {
    GameWorld world;
    sim::FlatTerrain ground{sim::provisionalGroundHeight()};
    client::Prediction prediction{ground};
    std::vector<net::Snapshot> inFlight; ///< snapshots pas encore arrivés au client

    ReconciledGame() {
        world.syncPlayers(std::vector<ConnectedPlayer>{{2, 1}});
        REQUIRE(prediction.start(world.snapshotFor(2, 0), 2));
    }

    template <typename Lost> void play(net::Tick ticks, std::size_t delay, Lost lost) {
        for (net::Tick tick = 1; tick <= ticks; ++tick) {
            REQUIRE(prediction.advance(varied(tick)));
            if (!lost(tick)) {
                world.receiveInputs(2, prediction.history().recentBatch());
            }
            world.step();
            deliver(world.snapshotFor(2, tick), delay);
        }
    }

    void deliver(net::Snapshot snapshot, std::size_t delay) {
        inFlight.push_back(std::move(snapshot));
        if (inFlight.size() > delay) {
            prediction.reconcile(inFlight.front(), 2);
            inFlight.erase(inFlight.begin());
        }
    }
};

} // namespace

TEST_CASE("Réconciliation : sans perte, le serveur confirme la prédiction (aucune correction)") {
    ReconciledGame game;
    game.play(200, 3, [](net::Tick) { return false; });

    CHECK(game.prediction.corrections() == 0);
    CHECK(*game.world.motionOf(2) == *game.prediction.state());
}

TEST_CASE("Réconciliation : des inputs perdus et mal devinés par le serveur sont corrigés") {
    ReconciledGame game;
    // Deux trous de 8 ticks : plus que le lot redondant (le serveur doit deviner), et l'input
    // change pendant ce temps (il devine faux).
    game.play(200, 3, [](net::Tick tick) {
        return (tick > 40 && tick <= 48) || (tick > 120 && tick <= 128);
    });

    CHECK(game.prediction.corrections() > 0);

    // Le client s'arrête. Le serveur applique un input par tick : après les trous, il a pris
    // du retard sur le client et doit d'abord vider sa file (sans deviner au-delà de 200).
    for (int i = 0; i < 20 && game.world.snapshotFor(2, 0).lastProcessedInput < 200; ++i) {
        game.world.step();
    }
    const net::Snapshot last = game.world.snapshotFor(2, 999);
    REQUIRE(last.lastProcessedInput == 200);
    game.prediction.reconcile(last, 2);
    CHECK(*game.prediction.state() == *game.world.motionOf(2));
}
