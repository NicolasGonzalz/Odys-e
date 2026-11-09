#include "Server.hpp"

#include "odyssee/core/Config.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace odyssee::server {

Server::Server(std::uint16_t port) : _port(port) {}

void Server::run() {
    using Clock = std::chrono::steady_clock;

    _running = true;
    std::cout << "[server] Odyssée v" << ODYSSEE_VERSION_STRING << " — port " << _port << ", "
              << kTickRate << " ticks/s\n";

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

    std::cout << "[server] Arrêt après " << tickIndex << " ticks.\n";
}

void Server::tick(std::uint64_t tickIndex) {
    // TODO(M1) : réception des paquets, application des inputs, simulation, envoi des snapshots.
    if (tickIndex % (kTickRate * 10) == 0) {
        std::cout << "[server] tick " << tickIndex << '\n';
    }
}

} // namespace odyssee::server
