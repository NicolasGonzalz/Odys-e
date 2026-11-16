#include "MapState.hpp"

#include <algorithm>
#include <variant>

namespace odyssee::debugclient {

bool MapState::onEvent(const net::GameEvent& event) {
    const auto* map = std::get_if<net::MapChunks>(&event);
    if (!map) {
        return false;
    }
    std::set<net::MapChunk>& layer = map->layer == net::MapLayer::Shared ? _shared : _pending;
    if (map->reset) {
        layer.clear();
    }
    layer.insert(map->chunks.begin(), map->chunks.end());
    return true;
}

std::size_t MapState::toReport() const {
    return static_cast<std::size_t>(
        std::count_if(_pending.begin(), _pending.end(),
                      [this](const net::MapChunk& chunk) { return !_shared.contains(chunk); }));
}

} // namespace odyssee::debugclient
