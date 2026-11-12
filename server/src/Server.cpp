#include "Server.hpp"

#include "ConnectionService.hpp"
#include "GameStep.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <thread>
#include <utility>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};

/// Ticks joués d'affilée, au plus, pour rattraper un retard (250 ms) : le reste est sauté.
constexpr int kMaxCatchUpTicks = 5;

} // namespace

Server::Server(std::uint16_t port, net::NetworkConditions network, GameWorld world)
    : _port(port), _network(network), _world(std::move(world)) {}

bool Server::run() {
    auto socket = net::UdpSocket::open(_port);
    if (!socket) {
        kLog.error("impossible d'ouvrir le port UDP {} : le serveur ne peut pas démarrer", _port);
        return false;
    }

    // Secret des défis de connexion : imprévisible, tiré une fois au démarrage.
    ConnectionService connections(std::move(*socket), SipKey::random(), _network);
    kLog.info("Odyssée v{} — port {}, {} ticks/s, {} joueurs max", ODYSSEE_VERSION_STRING, _port,
              kTickRate, kMaxPlayers);
    if (!_network.isPerfect()) {
        kLog.warn("simulation réseau active : {} ms ± {} ms, {} % de pertes, {} % de doublons",
                  _network.latency.count(), _network.jitter.count(), _network.lossPercent,
                  _network.duplicatePercent);
    }

    loop(connections);

    connections.shutdown();
    return true;
}

void Server::loop(ConnectionService& connections) {
    _running = true;
    std::uint64_t tickIndex = 0;
    auto nextTick = Clock::now();

    // Boucle à pas fixe : la simulation avance toujours de kTickDuration, quel que soit
    // le temps réel écoulé. Si le serveur prend du retard, il rattrape les ticks manqués —
    // mais pas sans limite : après une longue pause (machine suspendue, surcharge), enchaîner
    // des centaines de ticks d'un coup ne ferait que prolonger le retard.
    while (_running) {
        const auto now = Clock::now();
        int caughtUp = 0;
        while (nextTick <= now && _running && caughtUp < kMaxCatchUpTicks) {
            tick(connections, tickIndex++, now);
            nextTick += kTickDuration;
            ++caughtUp;
        }
        if (nextTick <= now) {
            const auto skipped = (now - nextTick) / kTickDuration + 1;
            kLog.warn("serveur en retard : {} tick(s) sautés", skipped);
            nextTick += skipped * kTickDuration;
        }
        waitUntil(connections, nextTick);
    }

    kLog.info("Arrêt après {} ticks.", tickIndex);
}

void Server::waitUntil(ConnectionService& connections, Clock::time_point nextTick) {
    // Avec la simulation réseau, des datagrammes retardés peuvent arriver à échéance entre deux
    // ticks : on se réveille pour eux, sinon leur délai serait arrondi au tick suivant (50 ms).
    while (true) {
        const auto due = connections.nextSimulatedDue();
        if (!due || *due >= nextTick) {
            break;
        }
        std::this_thread::sleep_until(*due);
        connections.flushSimulated(Clock::now());
    }
    std::this_thread::sleep_until(nextTick);
}

void Server::tick(ConnectionService& connections, std::uint64_t tickIndex,
                  Clock::time_point now) {
    // Les ticks réseau sont sur 32 bits : 6 ans à 20 ticks/s avant de reboucler.
    stepGame(connections, _world, static_cast<net::Tick>(tickIndex), now);

    if (tickIndex % (kTickRate * 10) == 0) {
        kLog.debug("tick {} — {} joueur(s)", tickIndex, connections.playerCount());
    }
}

} // namespace odyssee::server
