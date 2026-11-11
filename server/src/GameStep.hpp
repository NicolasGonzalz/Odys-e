#pragma once

#include "ConnectionService.hpp"
#include "GameWorld.hpp"

#include "odyssee/net/GameMessages.hpp"

#include <chrono>

namespace odyssee::server {

/// Un tick de jeu complet, dans l'ordre (voir docs/ARCHITECTURE.md) :
/// recevoir → appliquer les inputs → simuler → construire les snapshots → envoyer.
///
/// Séparé de Server pour être testé avec de vrais clients, sans la boucle temps réel.
void stepGame(ConnectionService& connections, GameWorld& world, net::Tick tick,
              std::chrono::steady_clock::time_point now);

} // namespace odyssee::server
