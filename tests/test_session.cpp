// Deux sessions reliées par un faux réseau (pertes, retards fixes) : pas de vrai socket, rapide
// et reproductible.
#include "odyssee/core/Config.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/Quantize.hpp"
#include "odyssee/net/Session.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

using namespace odyssee::net;
using namespace std::chrono_literals;

namespace {

using Clock = Session::Clock;

/// Livre un paquet à une session, comme le ferait la boucle réseau (découpage puis receive).
bool deliver(Session& to, const std::vector<std::byte>& bytes, Clock::time_point now) {
    const auto packet = parsePacket(bytes);
    REQUIRE(packet.has_value());
    return to.receive(*packet, now);
}

InputBatch inputsAt(Tick tick) {
    return InputBatch{tick, {InputCommand{quantizeAxis(1.0f), 0, quantizeYaw(45.0f), 0, 0}}};
}

std::vector<std::byte> payload(std::uint8_t value) { return {std::byte{value}}; }

} // namespace

TEST_CASE("Session : un message non fiable fait l'aller simple") {
    Session client;
    Session server;
    const auto now = Clock::now();

    client.queueUnreliable(inputsAt(10));
    CHECK(deliver(server, client.buildPacket(now), now));

    const auto messages = server.takeMessages();
    REQUIRE(messages.size() == 1);
    CHECK(std::get<InputBatch>(messages[0]) == inputsAt(10));
    CHECK(server.takeMessages().empty()); // déjà retirés
}

TEST_CASE("Session : les doublons et les paquets invalides sont ignorés") {
    Session client;
    Session server;
    const auto now = Clock::now();

    client.queueUnreliable(inputsAt(1));
    const auto packet = client.buildPacket(now);
    CHECK(deliver(server, packet, now));
    CHECK_FALSE(deliver(server, packet, now)); // même numéro : doublon
    CHECK(server.takeMessages().size() == 1);

    auto corrupted = client.buildPacket(now);
    corrupted.push_back(std::byte{0}); // octet en trop après le contenu
    CHECK_FALSE(deliver(server, corrupted, now));
    CHECK(server.stats().packetsIgnored == 2);
    CHECK(server.stats().packetsReceived == 1);
}

TEST_CASE("Session : l'aller-retour se mesure grâce aux acquittements") {
    Session client;
    Session server;
    const auto start = Clock::now();

    CHECK_FALSE(client.roundTripTime().has_value());
    const auto request = client.buildPacket(start);
    CHECK(deliver(server, request, start + 40ms));
    const auto answer = server.buildPacket(start + 40ms); // acquitte le paquet du client
    CHECK(deliver(client, answer, start + 80ms));

    REQUIRE(client.roundTripTime().has_value());
    CHECK(*client.roundTripTime() == 80ms);

    // Le même paquet acquitté de nouveau ne fausse pas la mesure.
    CHECK(deliver(client, server.buildPacket(start + 500ms), start + 500ms));
    CHECK(*client.roundTripTime() == 80ms);
}

TEST_CASE("Session : messages fiables livrés dans l'ordre malgré 30 % de pertes") {
    Session a;
    Session b;
    std::mt19937 random(7);
    std::bernoulli_distribution lost(0.3);
    auto now = Clock::now();

    for (std::uint8_t i = 0; i < 50; ++i) {
        REQUIRE(a.queueReliable(payload(i)));
    }

    std::vector<std::vector<std::byte>> received;
    for (int tick = 0; tick < 400 && received.size() < 50; ++tick) {
        const auto fromA = a.buildPacket(now);
        const auto fromB = b.buildPacket(now);
        if (!lost(random)) {
            deliver(b, fromA, now);
        }
        if (!lost(random)) {
            deliver(a, fromB, now);
        }
        for (auto& message : b.takeReliable()) {
            received.push_back(std::move(message));
        }
        now += 50ms;
    }

    REQUIRE(received.size() == 50);
    for (std::uint8_t i = 0; i < 50; ++i) {
        CHECK(received[i] == payload(i));
    }
}

TEST_CASE("Session : les non fiables qui ne tiennent pas dans le paquet sont abandonnés") {
    Session client;
    Session server;
    const auto now = Clock::now();

    Snapshot big{1, 0, {}};
    for (EntityId id = 0; id < odyssee::kMaxSnapshotEntities; ++id) {
        big.entities.push_back(EntityState{id, EntityKind::Player, {1, 2, 3}, 0, 0});
    }
    client.queueUnreliable(big);
    client.queueUnreliable(big); // le second ne tient plus

    const auto packet = client.buildPacket(now);
    CHECK(packet.size() <= odyssee::kMaxPacketSize);
    CHECK(client.stats().unreliableDropped == 1);
    CHECK(deliver(server, packet, now));
    CHECK(server.takeMessages().size() == 1);
}

TEST_CASE("Session : la file des messages reçus est bornée") {
    Session client;
    Session server;
    const auto now = Clock::now();

    for (Tick tick = 0; tick < Session::kMaxQueuedMessages + 10; ++tick) {
        client.queueUnreliable(inputsAt(tick));
        CHECK(deliver(server, client.buildPacket(now), now));
    }
    const auto messages = server.takeMessages();
    REQUIRE(messages.size() == Session::kMaxQueuedMessages);
    CHECK(std::get<InputBatch>(messages.back()).firstTick == Session::kMaxQueuedMessages + 9);
}

TEST_CASE("Session : un saut de numéro de paquet invraisemblable est ignoré") {
    Session client;
    Session server;
    const Clock::time_point t0{};
    REQUIRE(deliver(server, client.buildPacket(t0), t0));

    // Paquet valide par ailleurs, mais numéroté un demi-tour plus loin.
    const auto forged = game_packet::build(PacketHeader{odyssee::kProtocolVersion, 0x7000, 0, 0},
                                           GamePacketBody{});
    CHECK_FALSE(deliver(server, forged, t0));
    CHECK(server.stats().packetsIgnored == 1);

    CHECK(deliver(server, client.buildPacket(t0), t0)); // le vrai paquet suivant passe toujours
}

TEST_CASE("Session : un paquet sans le jeton de session est ignoré avant d'être analysé") {
    Session client(0xC0FFEE);
    Session server(0xC0FFEE);
    Session intruder(0xBAD);
    const Clock::time_point t0{};

    intruder.queueUnreliable(InputBatch{1, {InputCommand{}}});
    CHECK_FALSE(deliver(server, intruder.buildPacket(t0), t0));
    CHECK(server.takeMessages().empty());

    // Le numéro du paquet forgé n'a pas été enregistré : le vrai paquet 0 n'est pas un doublon.
    client.queueUnreliable(InputBatch{1, {InputCommand{}}});
    CHECK(deliver(server, client.buildPacket(t0), t0));
    CHECK(server.takeMessages().size() == 1);
}

TEST_CASE("Session : les messages d'un type non attendu sont ignorés sans être décodés") {
    Session client;
    Session server;
    server.acceptOnly(GameMessageType::Inputs);

    client.queueUnreliable(Snapshot{}); // un client n'a pas à envoyer de snapshot
    client.queueUnreliable(InputBatch{1, {InputCommand{}}});
    const Clock::time_point t0{};
    REQUIRE(deliver(server, client.buildPacket(t0), t0));

    const auto messages = server.takeMessages();
    REQUIRE(messages.size() == 1);
    CHECK(std::holds_alternative<InputBatch>(messages.front()));
}
