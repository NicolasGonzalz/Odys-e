#include "odyssee/core/Config.hpp"
#include "odyssee/net/Address.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <optional>
#include <thread>
#include <vector>

using odyssee::net::Address;
using odyssee::net::Received;
using odyssee::net::UdpSocket;

namespace {

/// En local, un datagramme arrive quasi instantanément, mais pas forcément avant
/// notre premier appel : on réessaie pendant au plus ~200 ms.
std::optional<Received> receiveWithin(UdpSocket& socket, std::span<std::byte> buffer) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (auto received = socket.receive(buffer)) {
            return received;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

std::vector<std::byte> bytes(std::string_view text) {
    std::vector<std::byte> out(text.size());
    std::memcpy(out.data(), text.data(), text.size());
    return out;
}

} // namespace

TEST_CASE("Address : lecture d'adresses valides") {
    const auto a = Address::parse("192.168.1.20", 27015);
    REQUIRE(a.has_value());
    CHECK(a->ip == 0xC0A80114u);
    CHECK(a->port == 27015);
    CHECK(a->toString() == "192.168.1.20:27015");
    CHECK(Address::parse("0.0.0.0", 1).has_value());
    CHECK(Address::parse("255.255.255.255", 1).has_value());
    CHECK(Address::loopback(42) == Address::parse("127.0.0.1", 42));
}

TEST_CASE("Address : les adresses invalides sont refusées") {
    for (const char* text : {"", "1.2.3", "1.2.3.4.5", "256.0.0.1", "1..2.3", "a.b.c.d", "1.2.3.4 ",
                             "01.2.3.4", "1.2.3.-4", "1234.1.1.1"}) {
        CAPTURE(text);
        CHECK_FALSE(Address::parse(text, 1).has_value());
    }
}

TEST_CASE("UdpSocket : open(0) obtient un port libre choisi par le système") {
    auto socket = UdpSocket::open();
    REQUIRE(socket.has_value());
    CHECK(socket->localPort() != 0);
}

TEST_CASE("UdpSocket : un port déjà utilisé est refusé") {
    auto first = UdpSocket::open();
    REQUIRE(first.has_value());
    CHECK_FALSE(UdpSocket::open(first->localPort()).has_value());
}

TEST_CASE("UdpSocket : receive ne bloque pas quand il n'y a rien") {
    auto socket = UdpSocket::open();
    REQUIRE(socket.has_value());
    std::array<std::byte, 64> buffer{};
    const auto start = std::chrono::steady_clock::now();
    CHECK_FALSE(socket->receive(buffer).has_value());
    CHECK(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(50));
}

TEST_CASE("UdpSocket : aller-retour d'un datagramme en local") {
    auto a = UdpSocket::open();
    auto b = UdpSocket::open();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());

    const auto message = bytes("bonjour Odyssée");
    REQUIRE(a->send(Address::loopback(b->localPort()), message));

    std::array<std::byte, odyssee::kMaxPacketSize> buffer{};
    const auto received = receiveWithin(*b, buffer);
    REQUIRE(received.has_value());
    CHECK(received->size == message.size());
    CHECK(std::memcmp(buffer.data(), message.data(), message.size()) == 0);
    CHECK(received->from == Address::loopback(a->localPort())); // on sait qui a envoyé

    // Réponse dans l'autre sens, à l'adresse de l'expéditeur.
    REQUIRE(b->send(received->from, bytes("salut")));
    const auto reply = receiveWithin(*a, buffer);
    REQUIRE(reply.has_value());
    CHECK(reply->size == 5);
}

TEST_CASE("UdpSocket : un datagramme trop gros est refusé à l'envoi") {
    auto socket = UdpSocket::open();
    REQUIRE(socket.has_value());
    const std::vector<std::byte> tooBig(odyssee::kMaxPacketSize + 1);
    CHECK_FALSE(socket->send(Address::loopback(socket->localPort()), tooBig));
}

TEST_CASE("UdpSocket : un datagramme plus grand que le tampon de lecture est ignoré") {
    auto a = UdpSocket::open();
    auto b = UdpSocket::open();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    const auto target = Address::loopback(b->localPort());
    REQUIRE(a->send(target, bytes("ce message est trop long")));
    REQUIRE(a->send(target, bytes("ok")));

    std::array<std::byte, 4> small{};
    const auto received = receiveWithin(*b, small);
    REQUIRE(received.has_value());
    CHECK(received->size == 2); // le premier a été sauté, le second est lu
}

TEST_CASE("UdpSocket : le déplacement transfère la propriété") {
    auto opened = UdpSocket::open();
    REQUIRE(opened.has_value());
    const auto port = opened->localPort();
    UdpSocket moved = std::move(*opened);
    CHECK(moved.localPort() == port);
    CHECK(opened->localPort() == 0); // l'ancien objet ne possède plus rien
}
