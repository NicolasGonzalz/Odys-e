#pragma once

#include "odyssee/net/GameMessages.hpp"

#include <span>

namespace odyssee::debugclient {

/// Dessine les sacs laissés à la mort des joueurs (T052), reçus dans le dernier snapshot : un
/// ballot brun noué, à la dernière position connue (ils ne bougent pas). On les ramasse en
/// passant dessus : c'est le serveur qui s'en charge.
void drawBags(std::span<const net::EntityState> entities);

} // namespace odyssee::debugclient
