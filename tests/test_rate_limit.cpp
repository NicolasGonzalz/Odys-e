// Limitation de débit : le seau à jetons, la table par adresse IP, et leur effet sur le vrai
// ConnectionService (flot de demandes de connexion, joueur trop bavard, plafond par tick).
#include "ConnectionService.hpp"
#include "IpRateLimiter.hpp"
#include "TokenBucket.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ClientConnection.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/Session.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <thread>

using namespace odyssee;
using namespace odyssee::net;
using odyssee::server::ConnectionService;
using odyssee::server::IpRateLimiter;
using odyssee::server::TokenBucket;
using namespace std::chrono_literals;

namespace {

using Clock = std::chrono::steady_clock;

const Clock::time_point t0{};

/// Laisse arriver ce qui vient d'être envoyé au serveur, puis le lui fait traiter.
void deliver(ConnectionService& service, Clock::time_point now) {
    std::this_thread::sleep_for(3ms);
    service.poll(now);
    std::this_thread::sleep_for(3ms);
}

int countReplies(UdpSocket& socket) {
    std::array<std::byte, kMaxPacketSize> buffer{};
    int count = 0;
    while (socket.receive(buffer)) {
        ++count;
    }
    return count;
}

/// Fait la poignée de main à la main depuis `socket` et renvoie le jeton de session.
SessionToken connectFrom(UdpSocket& socket, ConnectionService& service, Clock::time_point now) {
    ClientConnection connection("Teo", 5);
    const Address server = Address::loopback(service.port());
    connection.connect(now);
    std::array<std::byte, kMaxPacketSize> buffer{};
    for (int i = 0; i < 200 && connection.state() != ConnectionState::Connected; ++i) {
        if (const auto message = connection.nextOutgoing(now)) {
            (void)socket.send(server, buildControlPacket(*message));
        }
        deliver(service, now);
        while (const auto received = socket.receive(buffer)) {
            const auto packet = parsePacket(std::span(buffer).first(received->size));
            if (const auto message = decodeControl(packet->kind, packet->body)) {
                connection.onMessage(*message, now);
            }
        }
    }
    REQUIRE(connection.state() == ConnectionState::Connected);
    return *connection.sessionToken();
}

} // namespace

TEST_CASE("TokenBucket : rafale tolérée, puis débit soutenu") {
    TokenBucket bucket({3, 10}, t0);
    CHECK(bucket.take(t0));
    CHECK(bucket.take(t0));
    CHECK(bucket.take(t0));
    CHECK_FALSE(bucket.take(t0)); // rafale épuisée

    CHECK_FALSE(bucket.take(t0 + 99ms)); // 10 par seconde : un jeton toutes les 100 ms
    CHECK(bucket.take(t0 + 100ms));
    CHECK_FALSE(bucket.take(t0 + 100ms));

    CHECK_FALSE(bucket.isFull(t0 + 200ms));
    CHECK(bucket.isFull(t0 + 10s)); // jamais plus que la rafale
    CHECK(bucket.take(t0 + 10s));
    CHECK(bucket.take(t0 + 10s));
    CHECK(bucket.take(t0 + 10s));
    CHECK_FALSE(bucket.take(t0 + 10s));
    CHECK_FALSE(bucket.take(t0 + 5s)); // horloge qui recule : rien n'est ajouté
}

TEST_CASE("TokenBucket : les fractions de jeton ne sont pas perdues") {
    TokenBucket bucket({1, 4}, t0); // un jeton toutes les 250 ms
    REQUIRE(bucket.take(t0));
    int granted = 0;
    for (int ms = 1; ms <= 1000; ++ms) { // interrogé chaque milliseconde pendant une seconde
        granted += bucket.take(t0 + std::chrono::milliseconds{ms}) ? 1 : 0;
    }
    CHECK(granted == 4);
}

TEST_CASE("IpRateLimiter : un seau par adresse IP") {
    IpRateLimiter limiter({2, 1});
    CHECK(limiter.allow(1, t0));
    CHECK(limiter.allow(1, t0));
    CHECK_FALSE(limiter.allow(1, t0)); // l'adresse 1 a épuisé sa rafale
    CHECK(limiter.allow(2, t0));       // l'adresse 2 n'est pas concernée
    CHECK(limiter.allow(1, t0 + 1s));  // un jeton regagné
}

TEST_CASE("IpRateLimiter : mémoire bornée, les adresses inactives sont oubliées") {
    IpRateLimiter limiter({2, 1}, 100);
    for (std::uint32_t ip = 0; ip < 100; ++ip) {
        CHECK(limiter.allow(ip, t0));
    }
    // Table pleine d'adresses actives : une nouvelle adresse est refusée, les connues servies.
    CHECK_FALSE(limiter.allow(1000, t0));
    CHECK(limiter.allow(5, t0));
    CHECK(limiter.trackedCount() == 100);

    // Plus tard, les seaux sont redevenus pleins : ces adresses sont oubliées, la place revient.
    CHECK(limiter.allow(1000, t0 + 10s));
    CHECK(limiter.trackedCount() == 1);

    // Un flot d'adresses toutes différentes ne fait jamais grossir la table au-delà du plafond.
    for (std::uint32_t ip = 2000; ip < 12'000; ++ip) {
        (void)limiter.allow(ip, t0 + 11s);
    }
    CHECK(limiter.trackedCount() <= 100);
}

TEST_CASE("Débit : un flot de demandes de connexion ne reçoit qu'un nombre borné de réponses") {
    ConnectionService::Limits limits;
    limits.handshakePerIp = {8, 4};
    ConnectionService service(*UdpSocket::open(), 1, {}, limits);
    auto socket = *UdpSocket::open();
    const Address server = Address::loopback(service.port());
    const auto now = Clock::now();

    for (std::uint64_t salt = 0; salt < 60; ++salt) {
        REQUIRE(socket.send(server, buildControlPacket(ConnectRequest{"Flot", salt})));
    }
    deliver(service, now);
    CHECK(countReplies(socket) == 8); // la rafale, pas plus
    CHECK(service.stats().rateLimited == 52);

    // Une seconde plus tard, le débit soutenu : 4 de plus.
    for (std::uint64_t salt = 0; salt < 60; ++salt) {
        REQUIRE(socket.send(server, buildControlPacket(ConnectRequest{"Flot", salt})));
    }
    deliver(service, now + 1s);
    CHECK(countReplies(socket) == 4);
}

TEST_CASE("Débit : un joueur trop bavard est écarté, sans être déconnecté") {
    ConnectionService::Limits limits;
    limits.perPlayer = {10, 20};
    ConnectionService service(*UdpSocket::open(), 1, {}, limits);
    auto socket = *UdpSocket::open();
    auto now = Clock::now();
    Session session(connectFrom(socket, service, now));
    const Address server = Address::loopback(service.port());

    now += 1s;
    for (int i = 0; i < 50; ++i) {
        REQUIRE(socket.send(server, session.buildPacket(now)));
    }
    deliver(service, now);
    CHECK(service.session(0)->stats().packetsReceived == 10); // la rafale
    CHECK(service.stats().rateLimited == 40);
    CHECK(service.playerCount() == 1);

    // Les faux paquets « du joueur » (sans son jeton) ne puisent pas dans son débit.
    now += 1s;
    Session forged(0xBAD);
    for (int i = 0; i < 30; ++i) {
        REQUIRE(socket.send(server, forged.buildPacket(now)));
    }
    REQUIRE(socket.send(server, session.buildPacket(now)));
    deliver(service, now);
    CHECK(service.session(0)->stats().packetsReceived == 11);
}

TEST_CASE("Débit : nombre de datagrammes traités par tick plafonné") {
    ConnectionService::Limits limits;
    limits.maxDatagramsPerPoll = 20;
    ConnectionService service(*UdpSocket::open(), 1, {}, limits);
    auto socket = *UdpSocket::open();
    const Address server = Address::loopback(service.port());
    const std::array<std::byte, 4> noise{};

    for (int i = 0; i < 50; ++i) {
        REQUIRE(socket.send(server, noise));
    }
    deliver(service, Clock::now());
    CHECK(service.stats().datagramsHandled == 20); // le reste attend le tick suivant
    deliver(service, Clock::now());
    CHECK(service.stats().datagramsHandled == 40);
    deliver(service, Clock::now());
    CHECK(service.stats().datagramsHandled == 50);
}
