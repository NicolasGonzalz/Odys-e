#include "odyssee/client/TrafficMeter.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <initializer_list>

using odyssee::client::TrafficMeter;
using namespace std::chrono_literals;

namespace {

TrafficMeter::Clock::time_point start() {
    return TrafficMeter::Clock::time_point{} + 100s;
}

} // namespace

TEST_CASE("TrafficMeter : sans trafic, tout est à zéro") {
    const TrafficMeter meter;
    const auto stats = meter.stats(start());
    CHECK(stats.sentKilobytesPerSecond == 0.0);
    CHECK(stats.receivedKilobytesPerSecond == 0.0);
    CHECK(stats.incomingLossPercent == 0.0);
}

TEST_CASE("TrafficMeter : débit sur la dernière seconde, montant et descendant séparés") {
    TrafficMeter meter;
    const auto t0 = start();
    for (int i = 0; i < 20; ++i) { // 20 paquets par seconde, comme le jeu
        meter.onSent(256, t0 + i * 50ms);
        meter.onReceived(512, t0 + i * 50ms);
    }
    const auto stats = meter.stats(t0 + 950ms);
    CHECK(stats.sentKilobytesPerSecond == doctest::Approx(5.0));      // 20 × 256 o
    CHECK(stats.receivedKilobytesPerSecond == doctest::Approx(10.0)); // 20 × 512 o
}

TEST_CASE("TrafficMeter : ce qui a plus d'une seconde est oublié") {
    TrafficMeter meter;
    meter.onSent(10000, start());
    CHECK(meter.stats(start() + 2s).sentKilobytesPerSecond == 0.0);
}

TEST_CASE("TrafficMeter : pertes à la réception d'après les numéros de paquet") {
    TrafficMeter meter;
    const auto t0 = start();
    for (odyssee::net::Sequence s = 0; s < 20; ++s) {
        if (s != 5 && s != 12) { // deux paquets sur vingt jamais arrivés
            meter.onGamePacket(s, t0 + s * 40ms);
        }
    }
    CHECK(meter.stats(t0 + 800ms).incomingLossPercent == doctest::Approx(10.0));
}

TEST_CASE("TrafficMeter : un doublon ou un paquet en retard n'est pas une perte") {
    TrafficMeter meter;
    const auto t0 = start();
    using Sequences = std::initializer_list<odyssee::net::Sequence>;
    // 2 arrive après 3 ; 3 arrive en double.
    for (const odyssee::net::Sequence s : Sequences{0, 1, 3, 2, 3, 4}) {
        meter.onGamePacket(s, t0);
    }
    CHECK(meter.stats(t0).incomingLossPercent == doctest::Approx(0.0));
}

TEST_CASE("TrafficMeter : les numéros qui rebouclent (65535 puis 0) ne faussent pas les pertes") {
    TrafficMeter meter;
    const auto t0 = start();
    for (const odyssee::net::Sequence s :
         std::initializer_list<odyssee::net::Sequence>{65533, 65534, 65535, 0, 1, 2}) {
        meter.onGamePacket(s, t0);
    }
    CHECK(meter.stats(t0).incomingLossPercent == doctest::Approx(0.0));
}
