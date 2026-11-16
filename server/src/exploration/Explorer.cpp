#include "exploration/Explorer.hpp"

namespace odyssee::server {

std::vector<net::MapChunk> noteSurroundings(Explorer& explorer, const WorldMap& map,
                                            net::MapChunk here) {
    if (explorer.lastChunk == here) {
        return {};
    }
    explorer.lastChunk = here;
    std::vector<net::MapChunk> noted;
    for (const net::MapChunk& chunk : chunksAround(here, kSightChunks)) {
        if (!map.has(chunk) && explorer.pending.insert(chunk).second) {
            noted.push_back(chunk);
        }
    }
    return noted;
}

} // namespace odyssee::server
