#include "odyssee/client/Interpolation.hpp"

#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <vector>

using namespace odyssee;
using odyssee::client::Interpolation;

namespace {

constexpr net::EntityId kOther = 1;
constexpr std::int32_t kSpeed = 100; ///< le joueur avance de 100 unités (1/256 m) par tick

/// Un snapshot où le joueur kOther est en x = tick × kSpeed, regardant vers `yaw` degrés.
net::Snapshot at(net::Tick tick, float yaw = 0.0f) {
    net::Snapshot snapshot;
    snapshot.serverTick = tick;
    const std::int32_t x = static_cast<std::int32_t>(tick) * kSpeed;
    snapshot.entities.push_back(
        {kOther, net::EntityKind::Player, {x, 0, 0}, net::quantizeYaw(yaw), 0});
    return snapshot;
}

double shownX(const Interpolation& interpolation) {
    const auto shown = interpolation.sample();
    REQUIRE(shown.size() == 1);
    return shown.front().position[0];
}

/// Régime établi : un snapshot par tick, et l'horloge locale avance d'un tick à chaque fois.
void playSteadily(Interpolation& interpolation, net::Tick from, net::Tick to) {
    for (net::Tick tick = from; tick <= to; ++tick) {
        interpolation.push(at(tick));
        interpolation.advance(1.0);
    }
}

} // namespace

TEST_CASE("Interpolation : rien à montrer avant le premier snapshot") {
    const Interpolation interpolation;
    CHECK(interpolation.sample().empty());
    CHECK_FALSE(interpolation.renderTick().has_value());
}

TEST_CASE("Interpolation : en régime établi, l'affichage a kDelayTicks de retard") {
    Interpolation interpolation;
    playSteadily(interpolation, 1, 200);

    REQUIRE(interpolation.renderTick().has_value());
    CHECK(*interpolation.renderTick() ==
          doctest::Approx(200.0 - Interpolation::kDelayTicks).epsilon(0.01));
}

TEST_CASE("Interpolation : entre deux snapshots, la position est interpolée") {
    Interpolation interpolation;
    playSteadily(interpolation, 1, 100);
    interpolation.advance(0.4); // entre deux ticks

    const double t = *interpolation.renderTick();
    CHECK(t != doctest::Approx(std::round(t))); // vraiment entre deux snapshots
    CHECK(shownX(interpolation) == doctest::Approx(t * kSpeed));
}

TEST_CASE("Interpolation : l'orientation prend le chemin le plus court (350° → 10°)") {
    Interpolation interpolation;
    interpolation.push(at(10, 350.0f));
    for (net::Tick tick = 11; tick <= 20; ++tick) {
        interpolation.push(at(tick, 10.0f));
    }
    while (*interpolation.renderTick() < 10.5) {
        interpolation.advance(0.05);
    }

    const auto shown = interpolation.sample();
    REQUIRE(shown.size() == 1);
    const double yaw = std::fmod(shown.front().yawDegrees + 360.0, 360.0);
    CHECK((yaw > 350.0 || yaw < 10.0)); // passe par 0°, pas par 180°
}

TEST_CASE("Interpolation : le joueur local n'est pas montré (il est prédit)") {
    Interpolation interpolation;
    interpolation.push(at(1));
    CHECK(interpolation.sample(kOther).empty());
}

TEST_CASE("Interpolation : un doublon ou un snapshot trop vieux est ignoré") {
    Interpolation interpolation;
    playSteadily(interpolation, 1, 30);
    const double before = shownX(interpolation);

    net::Snapshot duplicate = at(30);
    duplicate.entities.front().position[0] = 999999;
    net::Snapshot late = at(2);
    late.entities.front().position[0] = 999999;
    interpolation.push(duplicate);
    interpolation.push(late);

    CHECK(shownX(interpolation) == doctest::Approx(before));
}

TEST_CASE("Interpolation : un snapshot perdu ne fait qu'allonger l'intervalle") {
    Interpolation interpolation;
    playSteadily(interpolation, 1, 50);
    for (net::Tick tick = 51; tick <= 60; ++tick) {
        if (tick != 55) {
            interpolation.push(at(tick));
        }
        interpolation.advance(1.0);
    }
    // Mouvement rectiligne : interpoler par-dessus le trou redonne la vraie position.
    CHECK(shownX(interpolation) == doctest::Approx(*interpolation.renderTick() * kSpeed));
}

TEST_CASE("Interpolation : sans saccades à 150 ms de latence, gigue et 5 % de pertes (T033)") {
    // Le serveur envoie un snapshot par tick (50 ms) ; réseau : 150 ms ± 40 ms de gigue, 5 % de
    // pertes, arrivées dans le désordre. Le client affiche à 60 images par seconde.
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> jitter(-40.0, 40.0);
    std::bernoulli_distribution lost(0.05);

    struct Arrival {
        double atMs;
        net::Tick tick;
    };
    std::vector<Arrival> arrivals;
    for (net::Tick tick = 1; tick <= 400; ++tick) {
        if (!lost(rng)) {
            arrivals.push_back({tick * 50.0 + 150.0 + jitter(rng), tick});
        }
    }
    std::sort(arrivals.begin(), arrivals.end(),
              [](const Arrival& a, const Arrival& b) { return a.atMs < b.atMs; });

    Interpolation interpolation;
    constexpr double kFrameMs = 1000.0 / 60.0;
    constexpr double kTrueStep = kSpeed * kFrameMs / 50.0; // vraie distance par image
    std::size_t next = 0;
    std::optional<double> previousX;
    double maxStep = 0.0;
    int backwards = 0;
    for (double now = 0.0; now < 20000.0; now += kFrameMs) {
        while (next < arrivals.size() && arrivals[next].atMs <= now) {
            interpolation.push(at(arrivals[next++].tick));
        }
        interpolation.advance(kFrameMs / 50.0);
        if (interpolation.sample().empty() || now < 2000.0) {
            continue; // on juge une fois l'horloge calée
        }
        const double x = shownX(interpolation);
        if (previousX) {
            maxStep = std::max(maxStep, x - *previousX);
            backwards += x < *previousX - 0.5 ? 1 : 0;
        }
        previousX = x;
    }

    // Lissé : jamais de recul, jamais plus de 1,5 × la vraie vitesse sur une image.
    CHECK(backwards == 0);
    CHECK(maxStep < 1.5 * kTrueStep);
}
