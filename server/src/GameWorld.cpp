#include "GameWorld.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/ProvisionalGround.hpp"
#include "odyssee/sim/Replication.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace odyssee::server {

namespace {

/// Point d'apparition sans carte : la salle des capsules du centre de stase
/// (tools/worldgen/world_layout.json, centre_stase), posé sur le sol provisoire partagé avec le
/// client (sim/ProvisionalGround.hpp).
SpawnPoint provisionalSpawn() {
    return {{net::quantizePosition(1900.0f), net::quantizePosition(2602.0f),
             sim::provisionalGroundHeight()},
            net::quantizeYaw(270.0f)}; // vers le nord : face à la sortie, vers le monde
}

/// Les joueurs apparaissent côte à côte d'ouest en est, à 2 m d'écart (entre les capsules).
constexpr std::int32_t kSpawnSpacing = 2 * 256;

/// Vrai si le tick suivant peut être deviné : le joueur a déjà envoyé un input, et on n'a pas
/// déjà trop deviné d'affilée (un client muet ne doit pas voir ses ticks avancer sans lui).
bool canGuess(net::Tick lastProcessedInput, std::uint32_t guessedInARow) {
    return lastProcessedInput != 0 && guessedInARow < GameWorld::kMaxGuessedInputs &&
           lastProcessedInput != std::numeric_limits<net::Tick>::max();
}

/// Input neutre : aucune touche de déplacement ni d'action, même orientation, et toujours
/// accroupi si le joueur l'était (il ne se relève pas tout seul).
net::InputCommand idleCommand(const sim::PlayerState& motion) {
    net::InputCommand command;
    command.yaw = motion.yaw;
    command.pitch = motion.pitch;
    if (motion.has(sim::kCrouched)) {
        command.buttons = static_cast<std::uint16_t>(net::InputButton::Crouch);
    }
    return command;
}

} // namespace

GameWorld::GameWorld()
    : GameWorld(std::make_unique<sim::FlatTerrain>(sim::provisionalGroundHeight()),
                provisionalSpawn()) {}

GameWorld::GameWorld(std::unique_ptr<const sim::Terrain> terrain, SpawnPoint spawn)
    : _terrain(std::move(terrain)), _spawn(spawn) {}

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
        if (slot) {
            stepPlayer(*slot);
        }
    }
}

void GameWorld::stepPlayer(PlayerState& player) const {
    if (const auto input = player.inputs.pop()) {
        apply(player, input->command, input->tick);
        player.guessedInARow = 0;
        catchUp(player);
        return;
    }
    if (canGuess(player.lastProcessedInput, player.guessedInARow)) {
        // Tick manquant deviné : s'il arrive plus tard, la file l'ignorera.
        const net::Tick tick = player.lastProcessedInput + 1;
        player.inputs.skip(tick);
        apply(player, player.lastCommand, tick);
        ++player.guessedInARow;
        return;
    }
    // Rien de plausible à rejouer : le joueur subit quand même la simulation (gravité, arrêt).
    player.motion = sim::step(player.motion, idleCommand(player.motion), *_terrain);
}

void GameWorld::catchUp(PlayerState& player) const {
    // Trop d'inputs en attente (après une coupure, le client a continué de jouer) : on en
    // applique un de plus ce tick-ci, sinon le serveur garderait ce retard pour toujours.
    if (player.inputs.size() >= kCatchUpBacklog) {
        if (const auto extra = player.inputs.pop()) {
            apply(player, extra->command, extra->tick);
        }
    }
}

void GameWorld::apply(PlayerState& player, const net::InputCommand& command,
                      net::Tick tick) const {
    player.motion = sim::step(player.motion, command, *_terrain);
    player.lastCommand = command;
    player.lastProcessedInput = tick;
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

GameWorld::PlayerState GameWorld::spawn(const ConnectedPlayer& player) const {
    // Décalage de -1,5 à +1,5 fois l'écart : les quatre joueurs centrés sur le point.
    const std::int32_t offset = (2 * std::int32_t{player.id} - 3) * kSpawnSpacing / 2;
    sim::PlayerState motion;
    motion.position = _spawn.position;
    motion.position[0] += offset;
    motion.yaw = _spawn.yaw;
    return PlayerState{player.generation, motion, 0, {}, {}, 0};
}

} // namespace odyssee::server
