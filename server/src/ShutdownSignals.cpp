#include "ShutdownSignals.hpp"

#include <csignal>

namespace odyssee::server {

namespace {

// Un gestionnaire de signal ne reçoit aucun contexte : le serveur visé est donc gardé ici.
Server* gServer = nullptr;

void onShutdownSignal(int /*signal*/) {
    if (gServer != nullptr) {
        // stop() ne fait qu'écrire un booléen atomique : c'est sûr dans un gestionnaire de signal.
        gServer->stop();
    }
}

} // namespace

void stopOnShutdownSignals(Server& server) {
    gServer = &server;
    std::signal(SIGINT, onShutdownSignal);
    std::signal(SIGTERM, onShutdownSignal);
}

} // namespace odyssee::server
