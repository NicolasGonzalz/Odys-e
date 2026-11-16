#pragma once

#include "MapState.hpp"

#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace odyssee::debugclient {

/// Ce que la carte plein écran montre en plus des carrés : les joueurs et les postes du dernier
/// snapshot, et qui l'on est.
struct MapMarkers {
    std::span<const net::EntityState> entities;
    std::optional<net::PlayerId> self;
};

/// La grande carte du client de debug (touche M, T074) : blanc = inconnu, couleur du relief =
/// carte commune, hachuré clair = ses souvenirs à reporter ; points des joueurs (rouge : soi),
/// carré brun : table de cartographie. `terrain` donne la couleur des carrés (nul : vert uni).
void drawMapScreen(const MapState& map, const MapMarkers& markers,
                   const world::WorldTerrain* terrain);

/// Rappel dans un coin de l'écran quand des souvenirs attendent d'être reportés.
void drawMapReminder(const MapState& map);

} // namespace odyssee::debugclient
