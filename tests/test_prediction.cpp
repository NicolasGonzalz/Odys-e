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

TEST_CASE("Prédiction : un second snapshot ne la fait pas repartir (réconciliation : T032)") {
    const sim::FlatTerrain ground(sim::provisionalGroundHeight());
    Prediction prediction(ground);
    REQUIRE(prediction.start(snapshotOf(standing()), kSelf));
    REQUIRE(prediction.advance(forward()));
    const sim::PlayerState predicted = *prediction.state();

    CHECK(prediction.start(snapshotOf(standing()), kSelf));
    CHECK(*prediction.state() == predicted);
}
