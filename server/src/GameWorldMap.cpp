// La carte dans le monde (T074) : exploration, report aux tables, carte commune.
#include "GameWorld.hpp"

#include "odyssee/core/Log.hpp"

#include <utility>
#include <vector>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"carte"};

} // namespace

std::optional<std::set<net::MapChunk>> GameWorld::pendingMapOf(net::PlayerId player) const {
    if (player >= kMaxPlayers || !_players[player]) {
        return std::nullopt;
    }
    const Explorer* explorer = _registry.get<Explorer>(*_players[player]);
    return explorer ? std::optional(explorer->pending) : std::nullopt;
}

void GameWorld::explore(const PlayerControl& control, const Motion& motion, Explorer& explorer) {
    const auto noted = noteSurroundings(explorer, _worldMap, chunkOf(motion.state.position));
    if (!noted.empty()) {
        sendTo(control.id, toEvents(net::MapLayer::Pending, false, noted));
    }
    if (!explorer.pending.empty() && _tables.isNear(_registry, motion.state.position)) {
        reportMap(control, explorer);
    }
}

void GameWorld::reportMap(const PlayerControl& control, Explorer& explorer) {
    const std::vector<net::MapChunk> remembered(explorer.pending.begin(), explorer.pending.end());
    const auto added = _worldMap.reveal(remembered);
    explorer.pending.clear();
    sendTo(control.id, toEvents(net::MapLayer::Pending, true, {}));
    kLog.info("{} reporte {} carré(s) sur la carte commune", control.name, added.size());
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (controlOf(id) && !added.empty()) {
            sendTo(id, toEvents(net::MapLayer::Shared, false, added));
        }
    }
}

void GameWorld::greetMap(net::PlayerId player) {
    if (!_players[player]) {
        return;
    }
    const std::vector<net::MapChunk> shared(_worldMap.chunks().begin(), _worldMap.chunks().end());
    sendTo(player, toEvents(net::MapLayer::Shared, true, shared));
    if (const Explorer* explorer = _registry.get<Explorer>(*_players[player])) {
        const std::vector<net::MapChunk> pending(explorer->pending.begin(),
                                                 explorer->pending.end());
        sendTo(player, toEvents(net::MapLayer::Pending, true, pending));
    }
}

void GameWorld::sendTo(net::PlayerId player, std::vector<net::MapChunks> events) {
    for (net::MapChunks& event : events) {
        _outbox[player].push_back(std::move(event));
    }
}

} // namespace odyssee::server
