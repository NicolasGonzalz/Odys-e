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
        if (const auto input = slot->inputs.pop()) {
            apply(*slot, *input);
        }
    }
}

net::Snapshot GameWorld::snapshotFor(net::PlayerId player, net::Tick serverTick) const {
    net::Snapshot snapshot{serverTick, 0, {}};
    if (player < kMaxPlayers && _players[player]) {
        snapshot.lastProcessedInput = _players[player]->lastProcessedInput;
    }
    for (const auto& slot : _players) {
        if (slot) {
            snapshot.entities.push_back(slot->entity);
        }
    }
    return snapshot;
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
    // Entités 0 à kMaxPlayers - 1 : les joueurs (numéro d'entité = numéro de joueur).
    const float offset = (static_cast<float>(player) - 1.5f) * kSpawnSpacing;
    net::EntityState entity{player, net::EntityKind::Player,
                            {net::quantizePosition(kSpawnX + offset),
                             net::quantizePosition(kSpawnY), net::quantizePosition(kSpawnZ)},
                            net::quantizeYaw(kSpawnYawDegrees), 0};
    return PlayerState{entity, 0, {}};
}

void GameWorld::apply(PlayerState& state, const InputQueue::Entry& input) {
    // Sans simulation (T029), seule l'orientation suit l'input : la caméra n'est pas soumise
    // aux collisions, le serveur peut la reprendre telle quelle.
    state.entity.yaw = input.command.yaw;
    state.lastProcessedInput = input.tick;
}

} // namespace odyssee::server
