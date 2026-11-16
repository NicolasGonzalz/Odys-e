#pragma once

#include "exploration/WorldMap.hpp"

#include "odyssee/net/GameEvents.hpp"

#include <optional>
#include <set>
#include <vector>

namespace odyssee::server {

/// Composant : ce qu'un joueur a vu en marchant (T074) et pas encore reporté à une table de
/// cartographie, ses « souvenirs ».
struct Explorer {
    std::set<net::MapChunk> pending;
    std::optional<net::MapChunk> lastChunk; ///< carré où il était au tick d'avant
};

/// Ce qu'on voit autour de soi : son carré et les huit voisins (environ 100 m).
inline constexpr std::int16_t kSightChunks = 1;

/// Le joueur est dans le carré `here` : il mémorise les carrés autour de lui qui ne sont ni sur la
/// carte commune ni déjà dans ses souvenirs. Renvoie les nouveaux (rien s'il n'a pas changé de
/// carré).
std::vector<net::MapChunk> noteSurroundings(Explorer& explorer, const WorldMap& map,
                                            net::MapChunk here);

} // namespace odyssee::server
