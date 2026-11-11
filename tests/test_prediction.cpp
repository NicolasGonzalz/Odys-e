#include "odyssee/client/Prediction.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/ProvisionalGround.hpp"
#include "odyssee/sim/Replication.hpp"

#include <doctest/doctest.h>

using namespace odyssee;
using odyssee::client::Prediction;

namespace {

constexpr net::EntityId kSelf = 2;

/// Un joueur au repos, debout sur le sol provisoire.
sim::PlayerState standing() {
    sim::PlayerState state;
    state.position = {net::quantizePosition(1900.0f), net::quantizePosition(2602.0f),
                      sim::provisionalGroundHeight()};
    state.yaw = net::quantizeYaw(270.0f);
    return state;
}

/// Ce que le serveur envoie : le joueur kSelf (et un autre), avec son mouvement à lui.
net::Snapshot snapshotOf(const sim::PlayerState& self) {
    net::Snapshot snapshot;
    snapshot.serverTick = 100;
    snapshot.entities.push_back(sim::toEntity(0, standing()));
    snapshot.entities.push_back(sim::toEntity(kSelf, self));
    snapshot.self = sim::toOwnMotion(self);
    return snapshot;
}

net::InputCommand forward() {
    net::InputCommand command;
    command.moveForward = net::quantizeAxis(1.0f);
    command.yaw = net::quantizeYaw(270.0f);
    return command;
}

} // namespace

TEST_CASE("Prédiction : pas de départ sans son joueur dans le snapshot") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);

    net::Snapshot other = snapshotOf(standing());
    other.entities.pop_back(); // seulement le joueur 0
    CHECK_FALSE(prediction.start(other, kSelf));
    CHECK_FALSE(prediction.started());
    CHECK_FALSE(prediction.advance(forward())); // rien à prédire tant qu'on ne sait pas où on est
    CHECK(prediction.history().empty());
}

TEST_CASE("Prédiction : repart exactement de l'état donné par le serveur") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);

    const sim::PlayerState server = standing();
    REQUIRE(prediction.start(snapshotOf(server), kSelf));
    REQUIRE(prediction.state().has_value());
    CHECK(*prediction.state() == server);
}

TEST_CASE("Prédiction : le joueur bouge tout de suite, sans attendre le serveur") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));

    const auto before = prediction.state()->position;
    REQUIRE(prediction.advance(forward()));
    CHECK(prediction.state()->position != before);
    CHECK(*prediction.state() == sim::step(standing(), forward(), ground));
}

TEST_CASE("Prédiction : les inputs joués sont numérotés à partir du tick 1, prêts à envoyer") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));

    for (int i = 0; i < 3; ++i) {
        REQUIRE(prediction.advance(forward()));
    }

    const net::InputBatch batch = prediction.history().recentBatch();
    CHECK(batch.firstTick == 1);
    CHECK(batch.commands.size() == 3);
}

TEST_CASE("Prédiction : un second snapshot ne la fait pas repartir (c'est reconcile qui recale)") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    REQUIRE(prediction.advance(forward()));
    const sim::PlayerState predicted = *prediction.state();

    CHECK(prediction.start(snapshotOf(standing()), kSelf));
    CHECK(*prediction.state() == predicted);
}

namespace {

/// Ce que dirait le serveur : le joueur dans l'état `server`, après son input n° `processed`.
net::Snapshot verdict(const sim::PlayerState& server, net::Tick processed) {
    net::Snapshot snapshot = snapshotOf(server);
    snapshot.lastProcessedInput = processed;
    return snapshot;
}

/// Décalé de `meters` vers l'est.
sim::PlayerState shifted(sim::PlayerState state, float meters) {
    state.position[0] += net::quantizePosition(meters);
    return state;
}

} // namespace

TEST_CASE("Réconciliation : repart de l'état du serveur et rejoue les inputs suivants") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    for (int i = 0; i < 5; ++i) {
        REQUIRE(prediction.advance(forward()));
    }

    // Selon le serveur, après l'input 2, le joueur était 1 m plus à l'est que prévu.
    sim::PlayerState server = standing();
    server = sim::step(server, forward(), ground);
    server = shifted(sim::step(server, forward(), ground), 1.0f);
    REQUIRE(prediction.reconcile(verdict(server, 2), kSelf));

    sim::PlayerState expected = server;
    for (int i = 0; i < 3; ++i) {
        expected = sim::step(expected, forward(), ground); // inputs 3, 4 et 5 rejoués
    }
    CHECK(*prediction.state() == expected);
    CHECK(prediction.corrections() == 1);
    CHECK(prediction.lastCorrection() == net::quantizePosition(1.0f));
}

TEST_CASE("Réconciliation : un snapshot plus ancien qu'un déjà utilisé est ignoré") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    for (int i = 0; i < 4; ++i) {
        REQUIRE(prediction.advance(forward()));
    }
    REQUIRE(prediction.reconcile(verdict(shifted(standing(), 0.5f), 3), kSelf));
    const sim::PlayerState after = *prediction.state();

    CHECK_FALSE(prediction.reconcile(verdict(shifted(standing(), 3.0f), 2), kSelf)); // en retard
    CHECK_FALSE(prediction.reconcile(verdict(shifted(standing(), 3.0f), 0), kSelf)); // rien traité
    CHECK(*prediction.state() == after);
}

TEST_CASE("Réconciliation : un petit écart est lissé, il fond en quelques ticks") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    REQUIRE(prediction.advance(net::InputCommand{}));
    const auto shown = prediction.displayPosition();

    REQUIRE(prediction.reconcile(verdict(shifted(standing(), 0.5f), 1), kSelf));
    CHECK(prediction.displayPosition() == shown); // pas de saut à l'écran au moment de corriger
    CHECK(prediction.state()->position[0] == shown[0] + net::quantizePosition(0.5f));

    for (int i = 0; i < 30; ++i) {
        REQUIRE(prediction.advance(net::InputCommand{}));
    }
    CHECK(prediction.displayPosition() == prediction.state()->position); // écart effacé
}

TEST_CASE("Réconciliation : un grand écart est recalé tout de suite") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    REQUIRE(prediction.advance(net::InputCommand{}));

    REQUIRE(prediction.reconcile(verdict(shifted(standing(), 10.0f), 1), kSelf));
    CHECK(prediction.displayPosition() == prediction.state()->position);
}

TEST_CASE("Réconciliation : le serveur en avance sur l'historique, on reprend son état") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    REQUIRE(prediction.advance(forward()));

    // Le serveur a deviné des inputs plus loin que ceux du client (client gelé un moment).
    const sim::PlayerState server = shifted(standing(), 1.0f);
    REQUIRE(prediction.reconcile(verdict(server, 4), kSelf));
    CHECK(*prediction.state() == server);
}
