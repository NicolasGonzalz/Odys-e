#include "GameWorld.hpp"

#include "odyssee/net/Quantize.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

/// Point d'apparition provisoire : la salle des capsules du centre de stase
/// (tools/worldgen/world_layout.json, centre_stase ; sol à ~10,6 m). Remplacé par celui de la
/// carte quand le serveur la chargera (T041).
constexpr float kSpawnX = 1900.0f;
constexpr float kSpawnY = 2598.0f;
constexpr float kSpawnZ = 10.6f;
constexpr float kSpawnYawDegrees = 90.0f; // vers le sud, face à la sortie

/// Les joueurs apparaissent côte à côte, à 2 m d'écart (entre les capsules).
constexpr float kSpawnSpacing = 2.0f;

} // namespace

GameWorld::GameWorld() : _terrain(net::quantizePosition(kSpawnZ)) {}

void GameWorld::syncPlayers(std::span<const net::PlayerId> connected) {
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        const bool isConnected =
            std::find(connected.begin(), connected.end(), id) != connected.end();
        auto& slot = _players[id];
        if (isConnected && !slot) {
            slot = spawn(id);
        } else if (!isConnected && slot) {
            slot.reset();
        }
    }
}

void GameWorld::receiveInputs(net::PlayerId player, const net::InputBatch& batch) {
    if (player < kMaxPlayers && _players[player]) {
        _players[player]->inputs.push(batch);
    }
}

void GameWorld::step() {
    for (auto& slot : _players) {
        if (!slot) {
            continue;
        }
        // Un joueur n'avance que lorsqu'un de ses inputs est appliqué : le client prédit un pas
        // par input, le serveur doit faire exactement les mêmes pas.
        if (const auto input = slot->inputs.pop()) {
            slot->motion = sim::step(slot->motion, input->command, _terrain);
            slot->lastProcessedInput = input->tick;
        }
    }
}

net::Snapshot GameWorld::snapshotFor(net::PlayerId player, net::Tick serverTick) const {
    net::Snapshot snapshot{serverTick, 0, {}, {}};
    if (player < kMaxPlayers && _players[player]) {
        const PlayerState& own = *_players[player];
        snapshot.lastProcessedInput = own.lastProcessedInput;
        snapshot.self = net::OwnMotion{own.motion.velocity, own.motion.stamina};
    }
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (_players[id]) {
            snapshot.entities.push_back(entityOf(id, _players[id]->motion));
        }
    }
    return snapshot;
}

std::optional<sim::PlayerState> GameWorld::motionOf(net::PlayerId player) const {
    if (player >= kMaxPlayers || !_players[player]) {
        return std::nullopt;
    }
    return _players[player]->motion;
}

bool GameWorld::hasPlayer(net::PlayerId player) const {
    return player < kMaxPlayers && _players[player].has_value();
}

std::size_t GameWorld::playerCount() const {
    return static_cast<std::size_t>(
        std::count_if(_players.begin(), _players.end(),
                      [](const auto& slot) { return slot.has_value(); }));
}

GameWorld::PlayerState GameWorld::spawn(net::PlayerId player) {
    const float offset = (static_cast<float>(player) - 1.5f) * kSpawnSpacing;
    sim::PlayerState motion;
    motion.position = {net::quantizePosition(kSpawnX + offset), net::quantizePosition(kSpawnY),
                       net::quantizePosition(kSpawnZ)};
    motion.yaw = net::quantizeYaw(kSpawnYawDegrees);
    return PlayerState{motion, 0, {}};
}

net::EntityState GameWorld::entityOf(net::PlayerId player, const sim::PlayerState& motion) {
    // Entités 0 à kMaxPlayers - 1 : les joueurs (numéro d'entité = numéro de joueur).
    return net::EntityState{player, net::EntityKind::Player, motion.position, motion.yaw,
                            motion.flags};
}

} // namespace odyssee::server
