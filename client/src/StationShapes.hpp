#pragma once

#include "odyssee/net/GameMessages.hpp"

#include <span>

namespace odyssee::debugclient {

/// Dessine les postes fixes du monde (T074) reçus dans le dernier snapshot : la table de
/// cartographie, une table brune à plateau clair. On s'en sert en passant à côté.
void drawStations(std::span<const net::EntityState> entities);

} // namespace odyssee::debugclient
