#pragma once

#include "Server.hpp"

namespace odyssee::server {

/// Demande l'arrêt propre de `server` à la réception de SIGINT (Ctrl+C) ou SIGTERM (systemd).
/// `server` doit vivre jusqu'à la fin du programme.
void stopOnShutdownSignals(Server& server);

} // namespace odyssee::server
