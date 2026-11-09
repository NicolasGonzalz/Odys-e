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
        ConnectRequest{"Nadia"}, Challenge{0x0123456789ABCDEFULL},
        ChallengeResponse{42},   Accepted{3},
        Refused{RefuseReason::ServerFull}, Disconnect{}, KeepAlive{}};

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

    const std::vector<std::byte> extra{std::byte{0}}; // octet en trop après un message vide
    CHECK_FALSE(decodeControl(PacketKind::KeepAlive, extra).has_value());
}

TEST_CASE("PacketFraming : paquets trop courts ou de type inconnu refusés") {
    auto bytes = buildControlPacket(KeepAlive{});
    bytes.back() = std::byte{99}; // type inconnu (le type est le dernier octet d'un KeepAlive)
    CHECK_FALSE(parsePacket(bytes).has_value());

    const std::vector<std::byte> tooShort(5);
    CHECK_FALSE(parsePacket(tooShort).has_value());
}
