#include "odyssee/net/ReliableSender.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <vector>

using namespace odyssee::net;
using namespace std::chrono_literals;

namespace {

const ReliableSender::Clock::time_point t0{};
constexpr std::size_t kBudget = 1000;

std::vector<std::byte> payload(std::size_t size) { return std::vector<std::byte>(size); }

} // namespace

TEST_CASE("ReliableSender : un message est envoyé puis attend son acquittement") {
    ReliableSender sender(100ms);
    REQUIRE(sender.enqueue(payload(4)) == MessageId{0});

    CHECK(sender.collectForPacket(1, t0, kBudget).size() == 1);
    CHECK(sender.collectForPacket(2, t0 + 50ms, kBudget).empty()); // trop tôt pour renvoyer
    CHECK(sender.pendingCount() == 1);
}

TEST_CASE("ReliableSender : renvoi après le délai si pas d'acquittement") {
    ReliableSender sender(100ms);
    (void)sender.enqueue(payload(4));
    (void)sender.collectForPacket(1, t0, kBudget);

    const auto resent = sender.collectForPacket(2, t0 + 100ms, kBudget);
    REQUIRE(resent.size() == 1);
    CHECK(resent[0].id == 0);
}

TEST_CASE("ReliableSender : l'acquittement du paquet confirme ses messages") {
    ReliableSender sender(100ms);
    (void)sender.enqueue(payload(4));
    (void)sender.enqueue(payload(4));
    (void)sender.collectForPacket(7, t0, kBudget);

    const std::vector<Sequence> acked{7};
    sender.onAcknowledged(acked);
    CHECK(sender.pendingCount() == 0);
    CHECK(sender.collectForPacket(8, t0 + 1s, kBudget).empty());
}

TEST_CASE("ReliableSender : l'acquittement d'un renvoi suffit aussi") {
    ReliableSender sender(100ms);
    (void)sender.enqueue(payload(4));
    (void)sender.collectForPacket(1, t0, kBudget);          // paquet 1 perdu
    (void)sender.collectForPacket(2, t0 + 100ms, kBudget); // renvoyé dans le paquet 2

    const std::vector<Sequence> acked{2};
    sender.onAcknowledged(acked);
    CHECK(sender.pendingCount() == 0);
}

TEST_CASE("ReliableSender : le budget du paquet est respecté") {
    ReliableSender sender(100ms);
    for (int i = 0; i < 3; ++i) {
        (void)sender.enqueue(payload(100)); // 104 octets encodés chacun
    }
    CHECK(sender.collectForPacket(1, t0, 1 + 2 * 104).size() == 2);
    CHECK(sender.collectForPacket(2, t0, kBudget).size() == 1); // le troisième part ensuite
}

TEST_CASE("ReliableSender : un message trop gros pour un paquet est refusé") {
    ReliableSender sender;
    CHECK_FALSE(sender.enqueue(payload(ReliableSender::maxPayloadSize() + 1)).has_value());
    CHECK(sender.enqueue(payload(ReliableSender::maxPayloadSize())).has_value());
}
