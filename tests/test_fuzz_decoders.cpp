// Octets aléatoires sur tous les décodeurs du protocole. Tirages reproductibles (graine fixe) :
// un échec se rejoue à l'identique. Ce qu'on vérifie : aucun plantage, aucune lecture hors
// bornes (les sanitizers de la CI veillent), et tout ce qui est accepté respecte les bornes
// annoncées et se réencode à l'identique.
#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/ClientConnection.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/net/GamePacket.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/ReliableCodec.hpp"
#include "odyssee/net/Session.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <variant>
#include <vector>

using namespace odyssee;
using namespace odyssee::net;

namespace {

/// Générateur du test : mt19937_64 donne la même suite partout (pas de distribution standard,
/// dont le résultat dépend de la bibliothèque).
struct Noise {
    std::mt19937_64 random;

    explicit Noise(std::uint64_t seed) : random(seed) {}

    std::size_t below(std::size_t bound) { return static_cast<std::size_t>(random() % bound); }

    std::vector<std::byte> bytes(std::size_t maxSize) {
        std::vector<std::byte> result(below(maxSize + 1));
        for (std::byte& byte : result) {
            byte = static_cast<std::byte>(random() & 0xFF);
        }
        return result;
    }

    /// Abîme `packet` : quelques octets changés, parfois tronqué ou rallongé.
    std::vector<std::byte> mutate(std::vector<std::byte> packet) {
        const std::size_t flips = 1 + below(4);
        for (std::size_t i = 0; i < flips && !packet.empty(); ++i) {
            packet[below(packet.size())] = static_cast<std::byte>(random() & 0xFF);
        }
        if (below(4) == 0) {
            packet.resize(below(packet.size() + 1));
        } else if (below(8) == 0) {
            packet.push_back(static_cast<std::byte>(random() & 0xFF));
        }
        return packet;
    }
};

void checkBounds(const ControlMessage& message) {
    if (const auto* accepted = std::get_if<Accepted>(&message)) {
        CHECK(accepted->playerId < kMaxPlayers);
    } else if (const auto* request = std::get_if<ConnectRequest>(&message)) {
        CHECK(request->playerName.size() <= 64);
    } else if (const auto* response = std::get_if<ChallengeResponse>(&message)) {
        CHECK(response->playerName.size() <= 64);
        CHECK(response->inviteCode.size() <= 64);
    }
}

void checkBounds(const GameMessage& message) {
    if (const auto* inputs = std::get_if<InputBatch>(&message)) {
        CHECK(inputs->firstTick >= 1);
        CHECK(inputs->commands.size() >= 1);
        CHECK(inputs->commands.size() <= kMaxInputsPerBatch);
    } else {
        CHECK(std::get<Snapshot>(message).entities.size() <= kMaxSnapshotEntities);
    }
}

/// Paquets valides de départ, pour les mutations : un de chaque sorte.
std::vector<std::vector<std::byte>> validPackets() {
    Session session(0x1122334455667788ULL);
    session.queueUnreliable(InputBatch{7, {InputCommand{10, -10, 500, -200, 3}, InputCommand{}}});
    Snapshot snapshot{12, 7, {EntityState{1, EntityKind::Player, {1, 2, 3}, 4, 5}}, {}, {}};
    session.queueUnreliable(snapshot);
    (void)session.queueReliable(std::vector<std::byte>(20, std::byte{0xAB}));

    return {buildControlPacket(ConnectRequest{"Nadia", 99}),
            buildControlPacket(Challenge{1234, 99}),
            buildControlPacket(ChallengeResponse{1234, 99, "Nadia", ""}),
            buildControlPacket(Accepted{2, 1234}),
            buildControlPacket(Refused{RefuseReason::ServerFull}),
            buildControlPacket(Disconnect{1234}),
            buildControlPacket(KeepAlive{1234}),
            session.buildPacket(Session::Clock::time_point{})};
}

} // namespace

TEST_CASE("Décodeurs : messages de connexion sur des octets aléatoires") {
    Noise noise(0x0D155EE1);
    for (int i = 0; i < 20'000; ++i) {
        const auto body = noise.bytes(120);
        const auto kind = static_cast<PacketKind>(1 + noise.below(8));
        const auto message = decodeControl(kind, body);
        if (!message) {
            continue;
        }
        CHECK(kindOf(*message) == kind);
        checkBounds(*message);
        // Ce qui est accepté se réencode et se relit à l'identique.
        CHECK(decodeControl(kind, encodeBody(*message)) == message);
    }
}

TEST_CASE("Décodeurs : messages de jeu sur des octets aléatoires") {
    Noise noise(0x0D155EE2);
    for (int i = 0; i < 20'000; ++i) {
        const auto body = noise.bytes(200);
        const auto type = static_cast<std::uint8_t>(noise.below(4)); // 0 et 3 : types inconnus
        const auto message = decodeGameMessage(type, body);
        if (!message) {
            continue;
        }
        CHECK(static_cast<std::uint8_t>(typeOf(*message)) == type);
        checkBounds(*message);
        CHECK(decodeGameMessage(type, encodeGameMessage(*message)) == message);
    }
}

TEST_CASE("Décodeurs : paquets entiers aléatoires") {
    Noise noise(0x0D155EE3);
    for (int i = 0; i < 20'000; ++i) {
        auto bytes = noise.bytes(300);
        if (bytes.size() > PacketHeader::kSize && i % 2 == 0) {
            // Un type de paquet valide une fois sur deux, pour aller plus loin que l'en-tête.
            bytes[PacketHeader::kSize] = static_cast<std::byte>(1 + noise.below(8));
        }
        const auto packet = parsePacket(bytes);
        if (!packet) {
            continue;
        }
        CHECK(packet->body.size() == bytes.size() - PacketHeader::kSize - 1);
        (void)sessionTokenOf(*packet);
        if (const auto message = decodeControl(packet->kind, packet->body)) {
            checkBounds(*message);
        }
        if (const auto body = game_packet::parse(packet->body)) {
            CHECK(body->reliable.size() <= reliable_codec::kMaxMessagesPerPacket);
            CHECK(body->unreliable.size() <= game_packet::kMaxUnreliablePerPacket);
        }

        ByteReader reader(packet->body);
        if (const auto reliable = reliable_codec::read(reader)) {
            CHECK(reliable->size() <= reliable_codec::kMaxMessagesPerPacket);
        }
    }
}

TEST_CASE("Décodeurs : paquets valides abîmés, donnés à une session et à une connexion") {
    Noise noise(0x0D155EE4);
    const auto originals = validPackets();
    const Session::Clock::time_point t0{};

    Session session(0x1122334455667788ULL);
    ClientConnection connection("Nadia", 1);
    connection.connect(t0);

    for (int i = 0; i < 20'000; ++i) {
        const auto bytes = noise.mutate(originals[noise.below(originals.size())]);
        const auto packet = parsePacket(bytes);
        if (!packet) {
            continue;
        }
        if (isControlPacketKind(packet->kind)) {
            if (const auto message = decodeControl(packet->kind, packet->body)) {
                checkBounds(*message);
                connection.onMessage(*message, t0);
            }
        } else {
            (void)session.receive(*packet, t0);
        }
        for (const GameMessage& message : session.takeMessages()) {
            checkBounds(message);
        }
        (void)session.takeReliable();
    }
    // Aucun de ces paquets ne portait le sel de cette connexion : elle n'a pas avancé.
    CHECK(connection.state() != ConnectionState::Connected);
}
