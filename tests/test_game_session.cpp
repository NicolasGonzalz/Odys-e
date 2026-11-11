// Test d'intégration de la partie : le vrai ConnectionService, le vrai GameWorld et de vrais
// clients sur la machine locale. Les clients se connectent, envoient leurs inputs à chaque tick
// et reçoivent l'état du monde (T026).
#include "ConnectionService.hpp"
#include "GameStep.hpp"
#include "GameWorld.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ClientConnection.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/Quantize.hpp"
#include "odyssee/net/Session.hpp"
#include "odyssee/net/UdpSocket.hpp"
#include "odyssee/sim/Movement.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace odyssee;
using namespace odyssee::net;
using odyssee::server::ConnectionService;
using odyssee::server::GameWorld;
using namespace std::chrono_literals;

namespace {

using Clock = std::chrono::steady_clock;

/// Un client de jeu minimal, comme le sera le plugin Unreal : poignée de main, puis à chaque
/// tick un paquet avec ses derniers inputs, et lecture des snapshots.
struct GameClient {
    ClientConnection connection;
    Session session;
    UdpSocket socket;
    Address server;

    float yawDegrees = 0.0f;  ///< où regarde ce joueur
    Tick nextInputTick = 1;   ///< ticks du client (indépendants de ceux du serveur)
    std::deque<InputCommand> recentInputs;
    std::optional<Snapshot> latest; ///< snapshot le plus récent reçu

    GameClient(std::string name, std::uint16_t serverPort)
        : connection(std::move(name)), socket(*UdpSocket::open()),
          server(Address::loopback(serverPort)) {}

    void step(Clock::time_point now) {
        while (const auto message = connection.nextOutgoing(now)) {
            (void)socket.send(server, buildControlPacket(*message));
        }
        if (connection.state() == ConnectionState::Connected) {
            sendInputs(now);
        }
        receiveAll(now);
        connection.update(now);
    }

    void sendInputs(Clock::time_point now) {
        InputCommand command;
        command.moveForward = quantizeAxis(1.0f);
        command.yaw = quantizeYaw(yawDegrees);
        recentInputs.push_back(command);
        if (recentInputs.size() > kMaxInputsPerBatch) {
            recentInputs.pop_front();
        }
        const Tick first = nextInputTick - static_cast<Tick>(recentInputs.size() - 1);
        session.queueUnreliable(InputBatch{first, {recentInputs.begin(), recentInputs.end()}});
        ++nextInputTick;

        (void)socket.send(server, session.buildPacket(now));
        connection.onGamePacketSent(now);
    }

    void receiveAll(Clock::time_point now) {
        std::array<std::byte, kMaxPacketSize> buffer{};
        while (const auto received = socket.receive(buffer)) {
            const auto packet = parsePacket(std::span(buffer).first(received->size));
            if (!packet) {
                continue;
            }
            if (isControlPacketKind(packet->kind)) {
                if (const auto message = decodeControl(packet->kind, packet->body)) {
                    connection.onMessage(*message, now);
                }
            } else if (session.receive(*packet, now)) {
                connection.onGamePacketReceived(now);
            }
        }
        for (auto& message : session.takeMessages()) {
            auto* snapshot = std::get_if<Snapshot>(&message);
            if (snapshot && (!latest || snapshot->serverTick > latest->serverTick)) {
                latest = std::move(*snapshot); // un snapshot plus vieux arrivé en retard est ignoré
            }
        }
    }

    [[nodiscard]] const EntityState* entity(EntityId id) const {
        if (!latest) {
            return nullptr;
        }
        const auto it = std::find_if(latest->entities.begin(), latest->entities.end(),
                                     [id](const EntityState& e) { return e.id == id; });
        return it == latest->entities.end() ? nullptr : &*it;
    }
};

/// Le serveur et ses clients avancent ensemble, tick par tick (temps simulé, vrais sockets).
struct Match {
    ConnectionService service;
    GameWorld world;
    Tick serverTick = 0;
    Clock::time_point now = Clock::now();

    explicit Match(NetworkConditions network = {})
        : service(*UdpSocket::open(), 4321, network) {}

    template <typename Done> void runUntil(std::vector<GameClient*> clients, Done done) {
        for (int i = 0; i < 300 && !done(); ++i) {
            for (GameClient* client : clients) {
                client->step(now);
            }
            std::this_thread::sleep_for(1ms); // laisse les paquets arriver
            server::stepGame(service, world, ++serverTick, now);
            std::this_thread::sleep_for(1ms);
            now += kTickDuration;
        }
    }

    void connect(GameClient& client) {
        client.connection.connect(now);
        runUntil({&client}, [&] { return client.connection.state() == ConnectionState::Connected; });
        REQUIRE(client.connection.state() == ConnectionState::Connected);
    }
};

} // namespace

TEST_CASE("Partie : deux joueurs se voient et le serveur applique leurs inputs") {
    Match match;
    GameClient ana("Ana", match.service.port());
    GameClient bilal("Bilal", match.service.port());
    ana.yawDegrees = 90.0f;
    bilal.yawDegrees = 180.0f;

    match.connect(ana);
    match.connect(bilal);
    const auto bilalId = *bilal.connection.playerId();

    // Ana voit Bilal regarder vers 180°, et le serveur a appliqué ses propres inputs récents.
    match.runUntil({&ana, &bilal}, [&] {
        const auto* seen = ana.entity(bilalId);
        return seen && seen->yaw == quantizeYaw(180.0f) && ana.latest->lastProcessedInput > 10;
    });

    REQUIRE(ana.latest.has_value());
    CHECK(ana.latest->entities.size() == 2);
    REQUIRE(ana.entity(bilalId) != nullptr);
    CHECK(ana.entity(bilalId)->yaw == quantizeYaw(180.0f));
    CHECK(ana.latest->lastProcessedInput > 10);
    CHECK(ana.latest->lastProcessedInput < ana.nextInputTick);
    CHECK(match.world.playerCount() == 2);

    // Le client a mesuré l'aller-retour, et personne n'a eu besoin de KeepAlive pour rester.
    CHECK(ana.session.roundTripTime().has_value());
    CHECK(ana.connection.state() == ConnectionState::Connected);
}

TEST_CASE("Partie : le serveur déplace le joueur selon ses inputs (simulation partagée)") {
    Match match;
    GameClient ana("Ana", match.service.port());
    ana.yawDegrees = 90.0f; // avance vers le sud (+y)
    match.connect(ana);
    const auto id = *ana.connection.playerId();

    match.runUntil({&ana}, [&] { return ana.entity(id) != nullptr; });
    REQUIRE(ana.entity(id) != nullptr);
    const auto start = ana.entity(id)->position;

    match.runUntil({&ana}, [&] { return ana.latest->lastProcessedInput > 60; });
    const auto* now = ana.entity(id);
    REQUIRE(now != nullptr);

    // ~3 s de marche à 3,5 m/s : plusieurs mètres vers le sud, ni à l'est, ni en hauteur.
    CHECK(now->position[1] - start[1] > quantizePosition(5.0f));
    CHECK(std::abs(now->position[0] - start[0]) < quantizePosition(0.1f));
    CHECK(now->position[2] == start[2]);
    CHECK(ana.latest->self.velocity[1] == sim::kDefaultTuning.walkSpeed);
    CHECK(ana.latest->self.stamina == sim::kDefaultTuning.maxStamina);
}

TEST_CASE("Partie : un joueur qui part disparaît des snapshots des autres") {
    Match match;
    GameClient ana("Ana", match.service.port());
    GameClient bilal("Bilal", match.service.port());
    match.connect(ana);
    match.connect(bilal);
    const auto bilalId = *bilal.connection.playerId();
    match.runUntil({&ana, &bilal}, [&] { return ana.entity(bilalId) != nullptr; });
    REQUIRE(ana.entity(bilalId) != nullptr);

    bilal.connection.disconnect();
    match.runUntil({&ana, &bilal}, [&] { return ana.latest->entities.size() == 1; });

    CHECK(ana.entity(bilalId) == nullptr);
    CHECK(match.service.playerCount() == 1);
    CHECK(match.world.playerCount() == 1);
}

TEST_CASE("Partie : l'état continue d'arriver sous un mauvais réseau côté serveur") {
    Match match(NetworkConditions{60ms, 20ms, 20.0, 5.0});
    GameClient ana("Ana", match.service.port());
    ana.yawDegrees = 270.0f;
    match.connect(ana);

    match.runUntil({&ana}, [&] { return ana.latest && ana.latest->lastProcessedInput > 40; });

    REQUIRE(ana.latest.has_value());
    CHECK(ana.latest->lastProcessedInput > 40);
    REQUIRE(ana.entity(*ana.connection.playerId()) != nullptr);
    CHECK(ana.entity(*ana.connection.playerId())->yaw == quantizeYaw(270.0f));
    CHECK(match.service.session(*ana.connection.playerId())->stats().packetsReceived > 40);
}

TEST_CASE("Partie : un paquet de jeu d'un inconnu ne crée pas de joueur") {
    Match match;
    auto stranger = *UdpSocket::open();
    Session session;
    session.queueUnreliable(InputBatch{1, {InputCommand{}}});
    (void)stranger.send(Address::loopback(match.service.port()), session.buildPacket(match.now));

    match.runUntil({}, [] { return false; });
    CHECK(match.service.playerCount() == 0);
    CHECK(match.world.playerCount() == 0);
}
