// Test d'intégration de la connexion : le vrai ConnectionService du serveur et de vrais clients,
// à travers de vrais sockets UDP en local. Le temps du jeu est simulé (pas besoin d'attendre
// 10 secondes pour tester un timeout) ; seuls de courts sleep laissent les paquets circuler.
#include "ConnectionService.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ClientConnection.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace odyssee;
using namespace odyssee::net;
using odyssee::server::ConnectionService;
using namespace std::chrono_literals;

namespace {

using Clock = std::chrono::steady_clock;

/// Un client de test : une ClientConnection branchée sur un vrai socket.
struct TestClient {
    ClientConnection connection;
    UdpSocket socket;
    Address server;

    TestClient(std::string name, std::uint16_t serverPort)
        : connection(std::move(name)), socket(*UdpSocket::open()),
          server(Address::loopback(serverPort)) {}

    /// Envoie ce que la connexion veut envoyer, lit les réponses, vérifie les délais.
    void step(Clock::time_point now) {
        while (const auto message = connection.nextOutgoing(now)) {
            (void)socket.send(server, buildControlPacket(*message));
        }
        std::array<std::byte, kMaxPacketSize> buffer{};
        while (const auto received = socket.receive(buffer)) {
            const auto packet = parsePacket(std::span(buffer).first(received->size));
            const auto message = packet ? decodeControl(packet->kind, packet->body) : std::nullopt;
            if (message) {
                connection.onMessage(*message, now);
            }
        }
        connection.update(now);
    }
};

/// Fait avancer serveur et clients jusqu'à ce que `done()` soit vrai (ou ~2 s de temps réel).
template <typename Done>
void runUntil(ConnectionService& service, std::vector<TestClient*> clients, Clock::time_point& now,
              Done done) {
    for (int i = 0; i < 400 && !done(); ++i) {
        for (TestClient* client : clients) {
            client->step(now);
        }
        std::this_thread::sleep_for(1ms); // laisse les paquets arriver
        service.poll(now);
        std::this_thread::sleep_for(1ms);
        now += 10ms;
    }
}

ConnectionService startServer() { return ConnectionService(*UdpSocket::open(), 1234); }

} // namespace

TEST_CASE("Connexion : un client se connecte au serveur") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();

    teo.connection.connect(now);
    runUntil(service, {&teo}, now,
             [&] { return teo.connection.state() == ConnectionState::Connected; });

    CHECK(teo.connection.state() == ConnectionState::Connected);
    CHECK(teo.connection.playerId() == PlayerId{0});
    CHECK(service.playerCount() == 1);
}

TEST_CASE("Connexion : le cinquième joueur est refusé (serveur plein)") {
    auto service = startServer();
    auto now = Clock::now();

    std::vector<std::unique_ptr<TestClient>> players;
    for (int i = 0; i < 5; ++i) {
        players.push_back(std::make_unique<TestClient>("J" + std::to_string(i), service.port()));
    }

    // Les quatre premiers se connectent un par un.
    for (int i = 0; i < 4; ++i) {
        TestClient& player = *players[static_cast<std::size_t>(i)];
        player.connection.connect(now);
        runUntil(service, {&player}, now,
                 [&] { return player.connection.state() == ConnectionState::Connected; });
        REQUIRE(player.connection.state() == ConnectionState::Connected);
    }

    TestClient& fifth = *players[4];
    fifth.connection.connect(now);
    runUntil(service, {&fifth}, now,
             [&] { return fifth.connection.state() == ConnectionState::Refused; });
    CHECK(fifth.connection.refuseReason() == RefuseReason::ServerFull);
    CHECK(service.playerCount() == 4);
}

TEST_CASE("Connexion : un client d'une autre version reçoit « mauvaise version »") {
    auto service = startServer();
    auto socket = *UdpSocket::open();

    const PacketHeader foreign{static_cast<std::uint16_t>(kProtocolVersion + 1), 0, 0, 0};
    const auto request = buildPacket(foreign, PacketKind::ConnectRequest, {});
    REQUIRE(socket.send(Address::loopback(service.port()), request));

    std::optional<ControlMessage> reply;
    std::array<std::byte, kMaxPacketSize> buffer{};
    for (int i = 0; i < 200 && !reply; ++i) {
        std::this_thread::sleep_for(1ms);
        service.poll(Clock::now());
        if (const auto received = socket.receive(buffer)) {
            const auto packet = parsePacket(std::span(buffer).first(received->size));
            REQUIRE(packet.has_value());
            reply = decodeControl(packet->kind, packet->body);
        }
    }
    CHECK(reply == ControlMessage{Refused{RefuseReason::WrongVersion}});
    CHECK(service.playerCount() == 0);
}

TEST_CASE("Connexion : déconnexion volontaire du client") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    teo.connection.connect(now);
    runUntil(service, {&teo}, now,
             [&] { return teo.connection.state() == ConnectionState::Connected; });
    REQUIRE(service.playerCount() == 1);

    teo.connection.disconnect();
    runUntil(service, {&teo}, now, [&] { return service.playerCount() == 0; });
    CHECK(service.playerCount() == 0);
}

TEST_CASE("Connexion : le serveur qui s'arrête prévient ses joueurs") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    teo.connection.connect(now);
    runUntil(service, {&teo}, now,
             [&] { return teo.connection.state() == ConnectionState::Connected; });
    REQUIRE(teo.connection.state() == ConnectionState::Connected);

    service.shutdown();
    for (int i = 0; i < 200 && teo.connection.state() != ConnectionState::Closed; ++i) {
        std::this_thread::sleep_for(1ms);
        teo.step(now);
    }
    CHECK(teo.connection.state() == ConnectionState::Closed);
}

TEST_CASE("Connexion : un client qui ne donne plus de nouvelles est retiré") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    teo.connection.connect(now);
    runUntil(service, {&teo}, now,
             [&] { return teo.connection.state() == ConnectionState::Connected; });
    REQUIRE(service.playerCount() == 1);

    // Le client se tait (plus de step) ; le serveur avance de plus de 10 secondes.
    service.poll(now + kConnectionTimeout + 1s);
    CHECK(service.playerCount() == 0);
}
