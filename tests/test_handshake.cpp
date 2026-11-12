// Test d'intégration de la connexion : le vrai ConnectionService du serveur et de vrais clients,
// à travers de vrais sockets UDP en local. Le temps du jeu est simulé (pas besoin d'attendre
// 10 secondes pour tester un timeout) ; seuls de courts sleep laissent les paquets circuler.
#include "ConnectionService.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ClientConnection.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/Session.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <memory>
#include <random>
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

/// Laisse arriver ce qui vient d'être envoyé au serveur, puis le lui fait traiter.
void deliver(ConnectionService& service, Clock::time_point now) {
    std::this_thread::sleep_for(2ms);
    service.poll(now);
    std::this_thread::sleep_for(2ms);
}

/// Connecte `client` (échec du test sinon).
void connect(ConnectionService& service, TestClient& client, Clock::time_point& now) {
    client.connection.connect(now);
    runUntil(service, {&client}, now,
             [&] { return client.connection.state() == ConnectionState::Connected; });
    REQUIRE(client.connection.state() == ConnectionState::Connected);
}

/// Nombre de datagrammes en attente sur `socket`.
int countReplies(UdpSocket& socket) {
    std::array<std::byte, kMaxPacketSize> buffer{};
    int count = 0;
    while (socket.receive(buffer)) {
        ++count;
    }
    return count;
}

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
    // Contenu quelconque : le serveur ne sait peut-être pas le lire, mais répond quand même.
    const std::vector<std::byte> body(16);
    const auto request = buildPacket(foreign, PacketKind::ConnectRequest, body);
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

TEST_CASE("Connexion : réussit malgré un mauvais réseau côté serveur") {
    // 30 % des réponses du serveur perdues, 60 ms ± 20 de délai, quelques doublons : les renvois
    // de la poignée de main (toutes les 250 ms) doivent suffire.
    const NetworkConditions bad{60ms, 20ms, 30.0, 10.0};
    ConnectionService service(*UdpSocket::open(), 1234, bad);
    TestClient lina("Lina", service.port());
    auto now = Clock::now();

    lina.connection.connect(now);
    runUntil(service, {&lina}, now,
             [&] { return lina.connection.state() == ConnectionState::Connected; });

    CHECK(lina.connection.state() == ConnectionState::Connected);
    CHECK(service.playerCount() == 1);
}

// --- Poignée de main hostile ------------------------------------------------------------------

TEST_CASE("Connexion hostile : le serveur ne répond jamais plus qu'il n'a reçu") {
    auto service = startServer();
    auto socket = *UdpSocket::open();
    const Address server = Address::loopback(service.port());
    const auto now = Clock::now();

    // Demande bien formée mais sans rembourrage : elle vaudrait un défi plus gros qu'elle.
    const auto padded = buildControlPacket(ConnectRequest{"Teo", 1});
    const std::span<const std::byte> unpadded = std::span(padded).first(PacketHeader::kSize + 14);
    REQUIRE(socket.send(server, unpadded));
    // Demande d'une autre version réduite à son en-tête : plus courte que le refus.
    const PacketHeader foreign{static_cast<std::uint16_t>(kProtocolVersion + 1), 0, 0, 0};
    REQUIRE(socket.send(server, buildPacket(foreign, PacketKind::ConnectRequest, {})));
    deliver(service, now);
    CHECK(countReplies(socket) == 0);

    // La même demande, rembourrée, reçoit bien son défi — pas plus gros qu'elle.
    REQUIRE(socket.send(server, padded));
    deliver(service, now);
    std::array<std::byte, kMaxPacketSize> buffer{};
    const auto reply = socket.receive(buffer);
    REQUIRE(reply.has_value());
    CHECK(reply->size <= padded.size());
    CHECK(service.playerCount() == 0);
}

TEST_CASE("Connexion hostile : un jeton de défi inventé n'ouvre pas de place") {
    auto service = startServer();
    auto socket = *UdpSocket::open();
    const Address server = Address::loopback(service.port());
    const auto now = Clock::now();

    std::mt19937_64 random(99);
    for (int i = 0; i < 200; ++i) {
        const ChallengeResponse guess{random(), random(), "Intrus", ""};
        REQUIRE(socket.send(server, buildControlPacket(guess)));
        if (i % 20 == 19) {
            deliver(service, now);
        }
    }
    CHECK(service.playerCount() == 0);
    CHECK(countReplies(socket) == 0);
}

TEST_CASE("Connexion hostile : des octets aléatoires ne font ni tomber ni réagir le serveur") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    connect(service, teo, now);

    // Depuis un inconnu, puis depuis l'adresse même du joueur connecté.
    auto stranger = *UdpSocket::open();
    const Address server = Address::loopback(service.port());
    std::mt19937_64 random(2024);
    for (int i = 0; i < 400; ++i) {
        std::vector<std::byte> noise(1 + random() % 200);
        for (std::byte& byte : noise) {
            byte = static_cast<std::byte>(random() & 0xFF);
        }
        if (i % 2 == 0 && noise.size() > 11) { // la moitié avec un en-tête et un type plausibles
            const auto header = buildPacket(PacketHeader{kProtocolVersion, 0, 0, 0},
                                            static_cast<PacketKind>(1 + random() % 8), {});
            std::copy(header.begin(), header.end(), noise.begin());
        }
        REQUIRE((i % 4 < 2 ? stranger : teo.socket).send(server, noise));
        if (i % 20 == 19) {
            deliver(service, now);
        }
    }
    CHECK(service.playerCount() == 1);
    CHECK(teo.connection.state() == ConnectionState::Connected);
    CHECK(service.session(0)->stats().packetsReceived == 0);
}

// --- Jeton de session -------------------------------------------------------------------------

TEST_CASE("Jeton de session : un paquet de jeu sans le bon jeton est ignoré avant tout effet") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    connect(service, teo, now);
    const Address server = Address::loopback(service.port());
    const SessionToken token = *teo.connection.sessionToken();

    // Même adresse que le joueur (comme une adresse usurpée), mais jeton faux ou absent.
    for (const SessionToken wrong : {SessionToken{0}, token + 1, ~token}) {
        Session forged(wrong);
        forged.queueUnreliable(InputBatch{1, {InputCommand{}}});
        REQUIRE(teo.socket.send(server, forged.buildPacket(now)));
    }
    deliver(service, now);
    const auto& stats = service.session(0)->stats();
    CHECK(stats.packetsReceived == 0);
    CHECK(stats.packetsIgnored == 0); // même pas présentés à la session
    CHECK(service.takeInputs(0).empty());

    // Ces paquets ne comptent pas comme signe de vie : le joueur muet est retiré à l'heure.
    service.poll(now + kConnectionTimeout + 1s);
    CHECK(service.playerCount() == 0);
}

TEST_CASE("Jeton de session : le bon jeton fait passer les paquets de jeu") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    connect(service, teo, now);

    Session session(*teo.connection.sessionToken());
    session.queueUnreliable(InputBatch{1, {InputCommand{}}});
    REQUIRE(teo.socket.send(Address::loopback(service.port()), session.buildPacket(now)));
    deliver(service, now);

    CHECK(service.session(0)->stats().packetsReceived == 1);
    CHECK(service.takeInputs(0).size() == 1);
}

TEST_CASE("Jeton de session : un Disconnect forgé ne déconnecte pas le joueur") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    connect(service, teo, now);
    const Address server = Address::loopback(service.port());
    const SessionToken token = *teo.connection.sessionToken();

    REQUIRE(teo.socket.send(server, buildControlPacket(Disconnect{token + 1})));
    REQUIRE(teo.socket.send(server, buildControlPacket(Disconnect{0})));
    deliver(service, now);
    CHECK(service.playerCount() == 1);

    REQUIRE(teo.socket.send(server, buildControlPacket(Disconnect{token})));
    deliver(service, now);
    CHECK(service.playerCount() == 0);
}

TEST_CASE("Jeton de session : un serveur usurpé ne peut pas fermer la connexion du client") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    connect(service, teo, now);

    teo.connection.onMessage(Disconnect{*teo.connection.sessionToken() + 1}, now);
    CHECK(teo.connection.state() == ConnectionState::Connected);
}

// --- Reconnexion ------------------------------------------------------------------------------

TEST_CASE("Reconnexion : depuis la même adresse, la session repart de zéro") {
    auto service = startServer();
    TestClient teo("Teo", service.port());
    auto now = Clock::now();
    connect(service, teo, now);
    const Address server = Address::loopback(service.port());
    const SessionToken oldToken = *teo.connection.sessionToken();
    const auto oldGeneration = service.connectedPlayers().front().generation;

    // Quelques paquets de jeu : la session du serveur a un historique (numéros reçus).
    Session oldSession(oldToken);
    for (int i = 0; i < 5; ++i) {
        REQUIRE(teo.socket.send(server, oldSession.buildPacket(now)));
    }
    deliver(service, now);
    REQUIRE(service.session(0)->stats().packetsReceived == 5);

    // Le client redémarre sur le même port et refait toute la poignée de main.
    connect(service, teo, now);
    const SessionToken newToken = *teo.connection.sessionToken();
    CHECK(newToken != oldToken);
    CHECK(service.playerCount() == 1);
    CHECK(service.connectedPlayers().front().generation != oldGeneration);
    CHECK(service.session(0)->stats().packetsReceived == 0); // session neuve

    // Ses numéros de paquet repartent de 0 : ils ne sont pas pris pour des doublons.
    Session newSession(newToken);
    REQUIRE(teo.socket.send(server, newSession.buildPacket(now)));
    // Un paquet de l'ancienne session, arrivé en retard, est ignoré.
    REQUIRE(teo.socket.send(server, oldSession.buildPacket(now)));
    deliver(service, now);
    CHECK(service.session(0)->stats().packetsReceived == 1);
}
