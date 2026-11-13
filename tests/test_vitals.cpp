#include "GameWorld.hpp"
#include "Vitals.hpp"

#include <doctest/doctest.h>

#include <vector>

using namespace odyssee;
using odyssee::server::ConnectedPlayer;
using odyssee::server::GameWorld;
using odyssee::server::stepVitals;
using odyssee::server::toOwnVitals;
using odyssee::server::Vitals;
using odyssee::server::VitalsTuning;

namespace {

constexpr std::int32_t kFull = VitalsTuning::kVitalScale;
constexpr VitalsTuning kTune{};

Vitals run(Vitals vitals, int ticks, bool sprinting = false) {
    for (int i = 0; i < ticks; ++i) {
        vitals = stepVitals(vitals, sprinting);
    }
    return vitals;
}

} // namespace

TEST_CASE("Besoins : la faim et la soif baissent à chaque tick, plus vite en courant") {
    const Vitals walked = run(Vitals{}, 100);
    CHECK(walked.hunger == kFull - 100 * kTune.hungerPerTick);
    CHECK(walked.thirst == kFull - 100 * kTune.thirstPerTick);
    CHECK(walked.health == kFull);

    const Vitals ran = run(Vitals{}, 100, true);
    CHECK(ran.hunger == kFull - 100 * (kTune.hungerPerTick + kTune.sprintExtraPerTick));
    CHECK(ran.thirst < walked.thirst);
}

TEST_CASE("Besoins : affamé ou assoiffé, la vie baisse ; les deux, deux fois plus vite") {
    const Vitals starving = run(Vitals{kFull, 0, kFull}, 10);
    CHECK(starving.health == kFull - 10 * kTune.starvingDamagePerTick);

    const Vitals both = run(Vitals{kFull, 0, 0}, 10);
    CHECK(both.health == kFull - 20 * kTune.starvingDamagePerTick);
}

TEST_CASE("Besoins : bien nourri, la vie remonte, sans dépasser le maximum") {
    const Vitals hurt{kFull / 2, kFull, kFull};
    CHECK(run(hurt, 10).health == kFull / 2 + 10 * kTune.regenPerTick);
    CHECK(run(hurt, 5'000).health == kFull); // pleine vers 3 300 ticks, jamais au-delà

    // Sous le seuil de faim, pas de récupération.
    const Vitals peckish{kFull / 2, kTune.regenAbove, kFull};
    CHECK(run(peckish, 1).health == kFull / 2);
}

TEST_CASE("Besoins : rien ne descend sous zéro") {
    const Vitals dead = run(Vitals{}, 10'000'000);
    CHECK(dead.hunger == 0);
    CHECK(dead.thirst == 0);
    CHECK(dead.health == 0);
}

TEST_CASE("Besoins : pourcentage montré au joueur, arrondi vers le haut") {
    CHECK(toOwnVitals(Vitals{}) == net::OwnVitals{100, 100, 100});
    CHECK(toOwnVitals(Vitals{0, 0, 0}) == net::OwnVitals{0, 0, 0});
    CHECK(toOwnVitals(Vitals{1, kFull / 2, kFull - 1}) == net::OwnVitals{1, 50, 100});
}

TEST_CASE("GameWorld : les joueurs ont faim et soif, et le snapshot le dit") {
    GameWorld world;
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);
    for (int tick = 0; tick < 20 * 60 * 5; ++tick) { // 5 min de jeu, sans input
        world.step();
    }

    const auto vitals = world.vitalsOf(0);
    REQUIRE(vitals);
    CHECK(vitals->hunger == kFull - 20 * 60 * 5 * kTune.hungerPerTick);
    const net::Snapshot snapshot = world.snapshotFor(0, 1);
    CHECK(snapshot.vitals == toOwnVitals(*vitals));
    CHECK(snapshot.vitals.hunger < 100);
    CHECK(snapshot.vitals.health == 100);
}
