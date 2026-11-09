// Test de bout en bout du canal fiable : deux « machines » simulées échangent à travers
// un faux réseau qui perd 20 % des paquets. Pas de vrai socket : rapide et reproductible.
#include "odyssee/core/Config.hpp"
#include "odyssee/net/Acknowledgements.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/PacketHeader.hpp"
#include "odyssee/net/ReceivedPackets.hpp"
#include "odyssee/net/ReliableCodec.hpp"
#include "odyssee/net/ReliableReceiver.hpp"
#include "odyssee/net/ReliableSender.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <optional>
#include <random>
#include <vector>

using namespace odyssee::net;
using namespace std::chrono_literals;

namespace {

using Packet = std::vector<std::byte>;
using TimePoint = ReliableSender::Clock::time_point;

/// Une extrémité de la connexion : numérote ses paquets, suit ce qu'elle reçoit, et fait
/// circuler les messages fiables.
struct Endpoint {
    Sequence nextSequence = 0;
    ReceivedPackets received;
    ReliableSender sender;
    ReliableReceiver receiver;
    std::vector<std::vector<std::byte>> delivered;

    Packet buildPacket(TimePoint now) {
        const PacketHeader header{odyssee::kProtocolVersion, nextSequence, received.ack(),
                                  received.ackBits()};
        const auto budget = odyssee::kMaxPacketSize - PacketHeader::kSize;
        const auto messages = sender.collectForPacket(nextSequence, now, budget);
        ++nextSequence;

        ByteWriter writer;
        header.write(writer);
        reliable_codec::write(writer, messages);
        return {writer.data().begin(), writer.data().end()};
    }

    void onPacket(const Packet& packet) {
        ByteReader reader(packet);
        const auto header = PacketHeader::read(reader);
        if (!header || received.record(header->sequence) != Arrival::New) {
            return;
        }

        const auto acked = acknowledgedSequences(header->ack, header->ackBits);
        sender.onAcknowledged(acked);

        const auto messages = reliable_codec::read(reader);
        REQUIRE(messages.has_value());
        for (const ReliableMessage& message : *messages) {
            receiver.receive(message);
        }
        for (auto& payload : receiver.takeDeliverable()) {
            delivered.push_back(std::move(payload));
        }
    }
};

/// Faux réseau : perd une proportion `lossRate` des paquets, de façon reproductible (graine fixe).
struct LossyNetwork {
    std::mt19937 random;
    std::bernoulli_distribution lost;

    LossyNetwork(double lossRate, unsigned seed) : random(seed), lost(lossRate) {}

    void transmit(const Packet& packet, Endpoint& to) {
        if (!lost(random)) {
            to.onPacket(packet);
        }
    }
};

std::vector<std::byte> numbered(int i) {
    return {static_cast<std::byte>(i & 0xFF), static_cast<std::byte>(i >> 8)};
}

/// Échanges client ↔ serveur à 20 ticks/s jusqu'à `maxTicks` ou jusqu'à ce que `done()` soit vrai.
template <typename Done>
void exchange(Endpoint& client, Endpoint& server, LossyNetwork& network, TimePoint& now,
              int maxTicks, Done done) {
    for (int tick = 0; tick < maxTicks && !done(); ++tick) {
        network.transmit(client.buildPacket(now), server);
        network.transmit(server.buildPacket(now), client);
        now += 50ms;
    }
}

/// Le client envoie 100 messages fiables au serveur à travers un réseau qui perd 20 % des paquets.
void checkDeliveryWithLoss(unsigned seed) {
    CAPTURE(seed);
    constexpr std::size_t kMessages = 100;

    Endpoint client;
    Endpoint server;
    LossyNetwork network(0.20, seed);
    for (std::size_t i = 0; i < kMessages; ++i) {
        REQUIRE(client.sender.enqueue(numbered(static_cast<int>(i))).has_value());
    }

    // 30 secondes simulées au maximum.
    TimePoint now{};
    exchange(client, server, network, now, 600,
             [&] { return server.delivered.size() >= kMessages; });

    REQUIRE(server.delivered.size() == kMessages);
    for (std::size_t i = 0; i < kMessages; ++i) {
        CHECK(server.delivered[i] == numbered(static_cast<int>(i))); // ordre exact
    }

    // Quelques échanges de plus pour que les derniers acquittements reviennent au client.
    exchange(client, server, network, now, 100, [&] { return client.sender.pendingCount() == 0; });
    CHECK(client.sender.pendingCount() == 0);
    CHECK(server.delivered.size() == kMessages); // aucun doublon livré
}

} // namespace

TEST_CASE("Canal fiable : 100 messages livrés dans l'ordre malgré 20 % de perte") {
    // Plusieurs graines = plusieurs schémas de perte, pour ne pas réussir « par chance ».
    for (const unsigned seed : {1u, 2u, 3u, 42u, 1234u}) {
        checkDeliveryWithLoss(seed);
    }
}
