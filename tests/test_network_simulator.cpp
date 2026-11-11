#include "odyssee/net/NetworkSimulator.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdint>
#include <vector>

using namespace odyssee::net;
using namespace std::chrono_literals;

namespace {

using Clock = NetworkSimulator::Clock;

const Address kPeer = Address::loopback(27015);

std::vector<std::byte> datagram(std::uint8_t marker) { return {std::byte{marker}}; }

/// Confie `count` datagrammes numérotés (0, 1, 2…) au même instant.
void submitNumbered(NetworkSimulator& simulator, int count, Clock::time_point now) {
    for (int i = 0; i < count; ++i) {
        simulator.submit(kPeer, datagram(static_cast<std::uint8_t>(i)), now);
    }
}

} // namespace

TEST_CASE("NetworkSimulator : sans condition, rien n'est simulé") {
    CHECK(NetworkConditions{}.isPerfect());
    CHECK_FALSE(NetworkConditions{1ms, 0ms, 0.0, 0.0}.isPerfect());
    CHECK_FALSE(NetworkConditions{0ms, 0ms, 0.0, 1.0}.isPerfect());

    NetworkSimulator simulator({}, 1);
    const auto now = Clock::now();
    simulator.submit(kPeer, datagram(7), now);
    const auto due = simulator.takeDue(now);
    REQUIRE(due.size() == 1);
    CHECK(due[0] == NetworkSimulator::Datagram{kPeer, datagram(7)});
}

TEST_CASE("NetworkSimulator : la latence retarde chaque datagramme") {
    NetworkSimulator simulator({100ms, 0ms, 0.0, 0.0}, 1);
    const auto start = Clock::now();
    submitNumbered(simulator, 3, start);

    CHECK(simulator.takeDue(start + 99ms).empty());
    CHECK(simulator.nextDue() == start + 100ms);
    const auto due = simulator.takeDue(start + 100ms);
    REQUIRE(due.size() == 3);
    CHECK(due[0].bytes == datagram(0)); // même échéance : l'ordre d'envoi est gardé
    CHECK(due[2].bytes == datagram(2));
    CHECK(simulator.inFlight() == 0);
    CHECK_FALSE(simulator.nextDue().has_value());
}

TEST_CASE("NetworkSimulator : la gigue reste dans l'intervalle et réordonne") {
    NetworkSimulator simulator({100ms, 30ms, 0.0, 0.0}, 42);
    const auto start = Clock::now();
    for (int i = 0; i < 200; ++i) {
        simulator.submit(kPeer, datagram(static_cast<std::uint8_t>(i)), start + i * 1ms);
    }

    CHECK(simulator.takeDue(start + 69ms).empty()); // 100 - 30 ms au plus tôt
    const auto due = simulator.takeDue(start + 199ms + 130ms);
    CHECK(due.size() == 200); // tout est arrivé au plus tard 130 ms après son envoi

    bool reordered = false;
    for (std::size_t i = 1; i < due.size(); ++i) {
        reordered = reordered || due[i].bytes[0] < due[i - 1].bytes[0];
    }
    CHECK(reordered);
}

TEST_CASE("NetworkSimulator : pertes de 0 %, 100 % et environ 25 %") {
    const auto now = Clock::now();

    NetworkSimulator none({0ms, 0ms, 0.0, 0.0}, 3);
    submitNumbered(none, 100, now);
    CHECK(none.takeDue(now).size() == 100);

    NetworkSimulator all({0ms, 0ms, 100.0, 0.0}, 3);
    submitNumbered(all, 100, now);
    CHECK(all.takeDue(now).empty());
    CHECK(all.stats().dropped == 100);

    NetworkSimulator quarter({0ms, 0ms, 25.0, 0.0}, 3);
    submitNumbered(quarter, 4000, now);
    const auto delivered = quarter.takeDue(now).size();
    CHECK(delivered > 2800); // 3000 attendus
    CHECK(delivered < 3200);
    CHECK(quarter.stats().submitted == 4000);
    CHECK(quarter.stats().dropped == 4000 - delivered);
}

TEST_CASE("NetworkSimulator : doublons") {
    const auto now = Clock::now();
    NetworkSimulator always({0ms, 0ms, 0.0, 100.0}, 5);
    submitNumbered(always, 10, now);
    CHECK(always.takeDue(now).size() == 20);
    CHECK(always.stats().duplicated == 10);
}

TEST_CASE("NetworkSimulator : même graine, mêmes tirages") {
    const NetworkConditions bad{80ms, 40ms, 20.0, 5.0};
    NetworkSimulator a(bad, 99);
    NetworkSimulator b(bad, 99);
    const auto now = Clock::now();
    submitNumbered(a, 300, now);
    submitNumbered(b, 300, now);
    CHECK(a.takeDue(now + 1s) == b.takeDue(now + 1s));
}

TEST_CASE("NetworkSimulator : valeurs absurdes ramenées dans les bornes") {
    NetworkSimulator simulator({-50ms, -5ms, 250.0, -3.0}, 1);
    CHECK(simulator.conditions().latency == 0ms);
    CHECK(simulator.conditions().jitter == 0ms);
    CHECK(simulator.conditions().lossPercent == 100.0);
    CHECK(simulator.conditions().duplicatePercent == 0.0);
}
