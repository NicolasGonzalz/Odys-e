#include "exploration/WorldMap.hpp"

#include <algorithm>
#include <limits>

namespace odyssee::server {

namespace {

constexpr std::int64_t kChunkUnits = std::int64_t{net::kMapChunkMeters} * 256;

/// Division arrondie vers le bas (les positions sous zéro restent dans le bon carré), bornée aux
/// coordonnées de carré possibles.
std::int16_t chunkCoordinate(std::int32_t units) {
    std::int64_t value = units / kChunkUnits;
    if (units % kChunkUnits != 0 && units < 0) {
        --value; // la division arrondit vers zéro : sous zéro, on veut le carré d'en dessous
    }
    return static_cast<std::int16_t>(std::clamp<std::int64_t>(
        value, std::numeric_limits<std::int16_t>::min(), std::numeric_limits<std::int16_t>::max()));
}

} // namespace

net::MapChunk chunkOf(const std::array<std::int32_t, 3>& position) {
    return {chunkCoordinate(position[0]), chunkCoordinate(position[1])};
}

std::vector<net::MapChunk> chunksAround(net::MapChunk center, std::int16_t radius) {
    std::vector<net::MapChunk> chunks;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            chunks.push_back({static_cast<std::int16_t>(center.x + dx),
                              static_cast<std::int16_t>(center.y + dy)});
        }
    }
    return chunks;
}

std::vector<net::MapChunk> WorldMap::reveal(std::span<const net::MapChunk> chunks) {
    std::vector<net::MapChunk> added;
    for (const net::MapChunk& chunk : chunks) {
        if (_chunks.insert(chunk).second) {
            added.push_back(chunk);
        }
    }
    return added;
}

std::vector<net::MapChunks> toEvents(net::MapLayer layer, bool reset,
                                     std::span<const net::MapChunk> chunks) {
    std::vector<net::MapChunks> events;
    for (std::size_t first = 0; first < chunks.size(); first += net::kMaxMapChunksPerEvent) {
        const std::size_t last = std::min(chunks.size(), first + net::kMaxMapChunksPerEvent);
        events.push_back({layer,
                          false,
                          {chunks.begin() + static_cast<std::ptrdiff_t>(first),
                           chunks.begin() + static_cast<std::ptrdiff_t>(last)}});
    }
    if (reset && events.empty()) {
        events.push_back({layer, true, {}});
    } else if (reset) {
        events.front().reset = true;
    }
    return events;
}

} // namespace odyssee::server
