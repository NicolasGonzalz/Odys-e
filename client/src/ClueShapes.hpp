#pragma once

#include "odyssee/net/GameMessages.hpp"

#include <span>

namespace odyssee::debugclient {

/// Dessine les indices (T049) reçus dans le dernier snapshot : un petit pupitre portant une
/// feuille, jaune vif tant qu'il n'est pas lu, grisé ensuite. On le lit au clic gauche en le
/// visant (c'est le serveur qui décide).
void drawClues(std::span<const net::EntityState> entities);

} // namespace odyssee::debugclient
