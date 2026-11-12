#include "odyssee/core/Config.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/PacketFraming.hpp"

#include <doctest/doctest.h>

#include <vector>

using namespace odyssee::net;

namespace {

/// Encode un message en paquet complet, puis le relit comme le ferait l'autre côté.
std::optional<ControlMessage> roundTrip(const ControlMessage& message) {
    const auto bytes = buildControlPacket(message);
    const auto packet = parsePacket(bytes);
    REQUIRE(packet.has_value());
    CHECK(packet->header.hasCurrentProtocol());
    CHECK(packet->kind == kindOf(message));
    return decodeControl(packet->kind, packet->body);
}

} // namespace

TEST_CASE("ControlMessages : aller-retour de chaque message") {
    const std::vector<ControlMessage> messages{
        ConnectRequest{"Nadia", 7},
        Challenge{0x0123456789ABCDEFULL, 7},
        ChallengeResponse{42, 7, "Nadia", ""},
        ChallengeResponse{42, 7, "Nadia", "ody-7kq2"},
        Accepted{3, 42},
        Refused{RefuseReason::ServerFull},
        Refused{RefuseReason::TooManyFromAddress},
        Refused{RefuseReason::WrongInviteCode},
        Disconnect{42},
        KeepAlive{42}};

    for (const ControlMessage& message : messages) {
        CAPTURE(message.index());
        CHECK(roundTrip(message) == message);
    }
}

TEST_CASE("ControlMessages : contenus invalides refusés") {
    const std::vector<std::byte> empty;
    CHECK_FALSE(decodeControl(PacketKind::Challenge, empty).has_value()); // jeton manquant

    const std::vector<std::byte> badPlayer{std::byte{9}}; // joueur 9 : il n'y a que 4 places
    CHECK_FALSE(decodeControl(PacketKind::Accepted, badPlayer).has_value());

    const std::vector<std::byte> badReason{std::byte{200}};
    CHECK_FALSE(decodeControl(PacketKind::Refused, badReason).has_value());

    const std::vector<std::byte> noToken; // KeepAlive et Disconnect portent le jeton de session
    CHECK_FALSE(decodeControl(PacketKind::KeepAlive, noToken).has_value());
    CHECK_FALSE(decodeControl(PacketKind::Disconnect, noToken).has_value());

    const std::vector<std::byte> extra(9); // octet en trop après le jeton
    CHECK_FALSE(decodeControl(PacketKind::KeepAlive, extra).has_value());
}

TEST_CASE("ControlMessages : la demande de connexion est rembourrée (anti-amplification)") {
    const auto request = buildControlPacket(ConnectRequest{"Teo", 1});
    const auto challenge = buildControlPacket(Challenge{1, 1});
    const auto refused = buildControlPacket(Refused{RefuseReason::ServerFull});
    CHECK(request.size() >= challenge.size());
    CHECK(request.size() >= refused.size());

    // Une demande sans rembourrage est refusée, même bien formée par ailleurs.
    const auto packet = parsePacket(request);
    REQUIRE(packet.has_value());
    CHECK(packet->body.size() == kConnectRequestBodySize);
    CHECK(decodeControl(PacketKind::ConnectRequest, packet->body).has_value());
    CHECK_FALSE(
        decodeControl(PacketKind::ConnectRequest, packet->body.first(13)).has_value());
}

TEST_CASE("PacketFraming : jeton de session des paquets d'après-connexion") {
    const auto keepAlive = buildControlPacket(KeepAlive{0xABCDEF});
    CHECK(sessionTokenOf(*parsePacket(keepAlive)) == std::uint64_t{0xABCDEF});

    const auto request = buildControlPacket(ConnectRequest{"Teo", 1});
    CHECK_FALSE(sessionTokenOf(*parsePacket(request)).has_value()); // pas un paquet de session

    const auto tooShort = buildPacket(PacketHeader{odyssee::kProtocolVersion, 0, 0, 0},
                                      PacketKind::Disconnect, {});
    CHECK_FALSE(sessionTokenOf(*parsePacket(tooShort)).has_value());
}

TEST_CASE("PacketFraming : paquets trop courts ou de type inconnu refusés") {
    auto bytes = buildControlPacket(KeepAlive{});
    bytes[PacketHeader::kSize] = std::byte{99}; // type inconnu (le type suit l'en-tête)
    CHECK_FALSE(parsePacket(bytes).has_value());

    const std::vector<std::byte> tooShort(5);
    CHECK_FALSE(parsePacket(tooShort).has_value());
}
