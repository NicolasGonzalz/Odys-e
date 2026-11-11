#include "Server.hpp"

#include "ConnectionService.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <random>
#include <thread>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};

/// Graine imprévisible pour les défis de connexion (un attaquant ne doit pas pouvoir les deviner).
std::uint64_t unpredictableSeed() {
    std::random_device device;
    return (std::uint64_t{device()} << 32) | device();
}

} // namespace

Server::Server(std::uint16_t port, net::NetworkConditions network)
    : _port(port), _network(network) {}

bool Server::run() {
    auto socket = net::UdpSocket::open(_port);
    if (!socket) {
        kLog.error("impossible d'ouvrir le port UDP {} : le serveur ne peut pas démarrer", _port);
        return false;
    }

    ConnectionService connections(std::move(*socket), unpredictableSeed(), _network);
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
    // le temps réel écoulé. Si le serveur prend du retard, il rattrape les ticks manqués.
    while (_running) {
        const auto now = Clock::now();
        while (nextTick <= now && _running) {
            tick(connections, tickIndex++, now);
            nextTick += kTickDuration;
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
    connections.poll(now);

    // TODO(M1) : application des inputs, simulation, envoi des snapshots.
    if (tickIndex % (kTickRate * 10) == 0) {
        kLog.debug("tick {} — {} joueur(s)", tickIndex, connections.playerCount());
    }
}

} // namespace odyssee::server
