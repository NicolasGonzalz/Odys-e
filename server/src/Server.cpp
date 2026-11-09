#include "Server.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"

#include <chrono>
#include <thread>

namespace odyssee::server {

namespace {
constexpr log::Logger kLog{"server"};
} // namespace

Server::Server(std::uint16_t port) : _port(port) {}

void Server::run() {
    using Clock = std::chrono::steady_clock;

    _running = true;
    kLog.info("Odyssée v{} — port {}, {} ticks/s", ODYSSEE_VERSION_STRING, _port, kTickRate);

    std::uint64_t tickIndex = 0;
    auto nextTick = Clock::now();

    // Boucle à pas fixe : la simulation avance toujours de kTickDuration, quel que soit
    // le temps réel écoulé. Si le serveur prend du retard, il rattrape les ticks manqués.
    while (_running) {
        const auto now = Clock::now();
        while (nextTick <= now && _running) {
            tick(tickIndex++);
            nextTick += kTickDuration;
        }
        std::this_thread::sleep_until(nextTick);
    }

    kLog.info("Arrêt après {} ticks.", tickIndex);
}

void Server::tick(std::uint64_t tickIndex) {
    // TODO(M1) : réception des paquets, application des inputs, simulation, envoi des snapshots.
    if (tickIndex % (kTickRate * 10) == 0) {
        kLog.debug("tick {}", tickIndex);
    }
}

} // namespace odyssee::server
