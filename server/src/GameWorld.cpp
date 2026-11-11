#include "GameWorld.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Replication.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

/// Point d'apparition provisoire : la salle des capsules du centre de stase
/// (tools/worldgen/world_layout.json, centre_stase ; terrain à 8,55 m, sol du bunker 30 cm
/// au-dessus). Remplacé par celui de la carte quand le serveur la chargera (T041).
constexpr float kSpawnX = 1900.0f;
constexpr float kSpawnY = 2602.0f;
constexpr float kSpawnZ = 8.85f;
constexpr float kSpawnYawDegrees = 270.0f; // vers le nord : face à la sortie, vers le monde

/// Les joueurs apparaissent côte à côte, à 2 m d'écart (entre les capsules).
constexpr float kSpawnSpacing = 2.0f;

} // namespace

GameWorld::GameWorld() : _terrain(net::quantizePosition(kSpawnZ)) {}

void GameWorld::syncPlayers(std::span<const ConnectedPlayer> connected) {
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        const auto player = std::find_if(connected.begin(), connected.end(),
                                         [id](const ConnectedPlayer& p) { return p.id == id; });
        auto& slot = _players[id];
        if (player == connected.end()) {
            slot.reset();
        } else if (!slot || slot->generation != player->generation) {
            slot = spawn(*player); // nouveau joueur, ou place reprise par une autre connexion
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
        snapshot.self = sim::toOwnMotion(own.motion);
    }
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (_players[id]) {
            // Entités 0 à kMaxPlayers - 1 : les joueurs (numéro d'entité = numéro de joueur).
            snapshot.entities.push_back(sim::toEntity(id, _players[id]->motion));
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

GameWorld::PlayerState GameWorld::spawn(const ConnectedPlayer& player) {
    const float offset = (static_cast<float>(player.id) - 1.5f) * kSpawnSpacing;
    sim::PlayerState motion;
    motion.position = {net::quantizePosition(kSpawnX + offset), net::quantizePosition(kSpawnY),
                       net::quantizePosition(kSpawnZ)};
    motion.yaw = net::quantizeYaw(kSpawnYawDegrees);
    return PlayerState{player.generation, motion, 0, {}};
}

} // namespace odyssee::server
