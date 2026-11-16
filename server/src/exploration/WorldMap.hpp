#pragma once

#include "odyssee/net/GameEvents.hpp"

#include <array>
#include <cstdint>
#include <set>
#include <span>
#include <vector>

namespace odyssee::server {

/// Le carré de carte (T074) qui contient un point du monde (1/256 m, x est, y sud).
[[nodiscard]] net::MapChunk chunkOf(const std::array<std::int32_t, 3>& position);

/// Les carrés à moins de `radius` carrés de `center` (un carré de côté 2 × radius + 1).
[[nodiscard]] std::vector<net::MapChunk> chunksAround(net::MapChunk center, std::int16_t radius);

/// La carte commune du monde (T074, docs/GAMEPLAY.md § 3) : les carrés reportés aux tables de
/// cartographie par tous les joueurs, plus la zone de départ, dessinée d'emblée.
class WorldMap {
  public:
    /// Ajoute ces carrés ; renvoie ceux qui étaient encore blancs (dans l'ordre donné).
    std::vector<net::MapChunk> reveal(std::span<const net::MapChunk> chunks);
    [[nodiscard]] bool has(net::MapChunk chunk) const { return _chunks.contains(chunk); }
    [[nodiscard]] const std::set<net::MapChunk>& chunks() const { return _chunks; }

  private:
    std::set<net::MapChunk> _chunks;
};

/// Découpe des carrés en événements MapChunks (au plus net::kMaxMapChunksPerEvent chacun). Avec
/// `reset`, le premier vide la couche chez le client, même s'il n'y a aucun carré.
[[nodiscard]] std::vector<net::MapChunks> toEvents(net::MapLayer layer, bool reset,
                                                   std::span<const net::MapChunk> chunks);

} // namespace odyssee::server
