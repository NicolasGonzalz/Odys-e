#include "odyssee/core/Config.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/net/GamePacket.hpp"
#include "odyssee/net/PacketFraming.hpp"

#include <doctest/doctest.h>

#include <vector>

using namespace odyssee::net;

namespace {

std::vector<std::byte> bytesOf(std::initializer_list<int> values) {
    std::vector<std::byte> bytes;
    for (const int value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

GamePacketBody sampleBody() {
    const Snapshot snapshot{77, 12, {EntityState{2, EntityKind::Player, {1, 2, 3}, 4, 0}}, {}, {}};
    return GamePacketBody{
        {ReliableMessage{5, bytesOf({1, 2, 3})}, ReliableMessage{6, {}}},
        {UnreliableMessage{static_cast<std::uint8_t>(GameMessageType::Snapshot),
                           encodeGameMessage(snapshot)}}};
}

} // namespace

TEST_CASE("GamePacket : aller-retour complet (en-tête, fiables, non fiables)") {
    const PacketHeader header{odyssee::kProtocolVersion, 300, 299, 0xF0F0F0F0};
    const GamePacketBody body = sampleBody();

    const auto bytes = game_packet::build(header, body);
    CHECK(bytes.size() == game_packet::packetSize(body));

    const auto packet = parsePacket(bytes);
    REQUIRE(packet.has_value());
    CHECK(packet->kind == PacketKind::Game);
    CHECK_FALSE(isControlPacketKind(packet->kind));
    CHECK(packet->header.sequence == 300);
    CHECK(packet->header.ackBits == 0xF0F0F0F0);

    const auto parsed = game_packet::parse(packet->body);
    REQUIRE(parsed.has_value());
    CHECK(*parsed == body);

    const auto& snapshot = parsed->unreliable.front();
    const auto message = decodeGameMessage(snapshot.type, snapshot.data);
    REQUIRE(message.has_value());
    CHECK(std::get<Snapshot>(*message).serverTick == 77);
}

TEST_CASE("GamePacket : paquet vide (seulement des acquittements)") {
    const PacketHeader header{odyssee::kProtocolVersion, 1, 0, 0};
    const auto bytes = game_packet::build(header, {});
    const auto packet = parsePacket(bytes);
    REQUIRE(packet.has_value());
    CHECK(game_packet::parse(packet->body) == GamePacketBody{});
}

TEST_CASE("GamePacket : un type non fiable inconnu se lit sans casser le paquet") {
    GamePacketBody body = sampleBody();
    body.unreliable.push_back(UnreliableMessage{250, bytesOf({9, 9})});
    const auto bytes = game_packet::build(PacketHeader{odyssee::kProtocolVersion, 1, 0, 0}, body);

    const auto parsed = game_packet::parse(parsePacket(bytes)->body);
    REQUIRE(parsed.has_value());
    CHECK(parsed->unreliable.size() == 2);
    CHECK_FALSE(
        decodeGameMessage(parsed->unreliable[1].type, parsed->unreliable[1].data).has_value());
}

TEST_CASE("GamePacket : contenus tronqués ou incohérents refusés") {
    const auto bytes = game_packet::build(PacketHeader{odyssee::kProtocolVersion, 1, 0, 0},
                                          sampleBody());
    const auto body = parsePacket(bytes)->body;

    for (std::size_t cut = 0; cut < body.size(); ++cut) {
        CAPTURE(cut);
        CHECK_FALSE(game_packet::parse(body.first(cut)).has_value());
    }

    std::vector<std::byte> extra(body.begin(), body.end());
    extra.push_back(std::byte{0});
    CHECK_FALSE(game_packet::parse(extra).has_value());

    // Compteur non fiable au-delà du maximum : 0 fiable, puis 200 messages annoncés.
    CHECK_FALSE(game_packet::parse(bytesOf({0, 200})).has_value());
}

TEST_CASE("GamePacket : un paquet de jeu n'est pas un message de connexion") {
    const auto bytes = game_packet::build(PacketHeader{odyssee::kProtocolVersion, 1, 0, 0},
                                          sampleBody());
    const auto packet = parsePacket(bytes);
    REQUIRE(packet.has_value());
    CHECK_FALSE(decodeControl(packet->kind, packet->body).has_value());
}
