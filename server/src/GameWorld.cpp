#include "GameWorld.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/ProvisionalGround.hpp"
#include "odyssee/sim/Replication.hpp"

#include <algorithm>
#include <limits>
#include <optional>
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

GameWorld::GameWorld(std::unique_ptr<const sim::Terrain> terrain, SpawnPoint spawn,
                     std::uint64_t worldFingerprint)
    : _terrain(std::move(terrain)), _spawn(spawn), _worldFingerprint(worldFingerprint) {}

void GameWorld::syncPlayers(std::span<const ConnectedPlayer> connected) {
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        const auto player = std::find_if(connected.begin(), connected.end(),
                                         [id](const ConnectedPlayer& p) { return p.id == id; });
        const PlayerControl* current = controlOf(id);
        if (player == connected.end()) {
            despawn(id);
        } else if (!current || current->generation != player->generation) {
            despawn(id); // place reprise par une autre connexion : on repart de zéro
            spawn(*player);
        }
    }
}

void GameWorld::receiveInputs(net::PlayerId player, const net::InputBatch& batch) {
    if (player < kMaxPlayers && _players[player]) {
        _registry.get<PlayerControl>(*_players[player])->inputs.push(batch);
    }
}

void GameWorld::step() {
    // Système « joueurs » : chaque entité pilotée par un joueur avance d'un tick.
    _registry.each<PlayerControl, Motion>([this](ecs::Entity, PlayerControl& control,
                                                 Motion& motion) { stepPlayer(control, motion); });

    // Système « besoins vitaux » (T051) : faim et soif baissent, la vie suit. Courir coûte plus.
    _registry.each<Vitals, Motion>([](ecs::Entity, Vitals& vitals, const Motion& motion) {
        vitals = stepVitals(vitals, motion.state.has(sim::kSprinting));
    });
}

void GameWorld::stepPlayer(PlayerControl& control, Motion& motion) const {
    if (const auto input = control.inputs.pop()) {
        apply(control, motion, input->command, input->tick);
        control.guessedInARow = 0;
        catchUp(control, motion);
        return;
    }
    if (canGuess(control.lastProcessedInput, control.guessedInARow)) {
        // Tick manquant deviné : s'il arrive plus tard, la file l'ignorera.
        const net::Tick tick = control.lastProcessedInput + 1;
        control.inputs.skip(tick);
        apply(control, motion, control.lastCommand, tick);
        ++control.guessedInARow;
        return;
    }
    // Rien de plausible à rejouer : le joueur subit quand même la simulation (gravité, arrêt).
    motion.state = sim::step(motion.state, idleCommand(motion.state), *_terrain);
}

void GameWorld::catchUp(PlayerControl& control, Motion& motion) const {
    // Trop d'inputs en attente (après une coupure, le client a continué de jouer) : on en
    // applique un de plus ce tick-ci, sinon le serveur garderait ce retard pour toujours.
    if (control.inputs.size() >= kCatchUpBacklog) {
        if (const auto extra = control.inputs.pop()) {
            apply(control, motion, extra->command, extra->tick);
        }
    }
}

void GameWorld::apply(PlayerControl& control, Motion& motion, const net::InputCommand& command,
                      net::Tick tick) const {
    motion.state = sim::step(motion.state, command, *_terrain);
    control.lastCommand = command;
    control.lastProcessedInput = tick;
}

net::Snapshot GameWorld::snapshotFor(net::PlayerId player, net::Tick serverTick) const {
    net::Snapshot snapshot{serverTick, 0, {}, {}, {}};
    if (const PlayerControl* own = controlOf(player)) {
        snapshot.lastProcessedInput = own->lastProcessedInput;
        snapshot.self = sim::toOwnMotion(motionComponentOf(player)->state);
        if (const auto vitals = vitalsOf(player)) {
            snapshot.vitals = toOwnVitals(*vitals);
        }
    }
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (const Motion* motion = motionComponentOf(id)) {
            // Entités 0 à kMaxPlayers - 1 : les joueurs (numéro d'entité = numéro de joueur).
            snapshot.entities.push_back(sim::toEntity(id, motion->state));
        }
    }
    return snapshot;
}

std::optional<sim::PlayerState> GameWorld::motionOf(net::PlayerId player) const {
    const Motion* motion = motionComponentOf(player);
    return motion ? std::optional<sim::PlayerState>(motion->state) : std::nullopt;
}

std::optional<Vitals> GameWorld::vitalsOf(net::PlayerId player) const {
    if (player >= kMaxPlayers || !_players[player]) {
        return std::nullopt;
    }
    const Vitals* vitals = _registry.get<Vitals>(*_players[player]);
    return vitals ? std::optional<Vitals>(*vitals) : std::nullopt;
}

bool GameWorld::hasPlayer(net::PlayerId player) const {
    return controlOf(player) != nullptr;
}

std::size_t GameWorld::playerCount() const {
    return static_cast<std::size_t>(std::count_if(
        _players.begin(), _players.end(), [](const auto& entity) { return entity.has_value(); }));
}

void GameWorld::spawn(const ConnectedPlayer& player) {
    // Décalage de -1,5 à +1,5 fois l'écart : les quatre joueurs centrés sur le point.
    const std::int32_t offset = (2 * std::int32_t{player.id} - 3) * kSpawnSpacing / 2;
    sim::PlayerState state;
    state.position = _spawn.position;
    state.position[0] += offset;
    state.yaw = _spawn.yaw;

    const ecs::Entity entity = _registry.create();
    _registry.add<Motion>(entity, Motion{state});
    _registry.add<Vitals>(entity); // plein de vie, rassasié, désaltéré
    PlayerControl control;
    control.id = player.id;
    control.generation = player.generation;
    _registry.add<PlayerControl>(entity, std::move(control));
    _players[player.id] = entity;
}

void GameWorld::despawn(net::PlayerId player) {
    if (_players[player]) {
        _registry.destroy(*_players[player]);
        _players[player].reset();
    }
}

const PlayerControl* GameWorld::controlOf(net::PlayerId player) const {
    if (player >= kMaxPlayers || !_players[player]) {
        return nullptr;
    }
    return _registry.get<PlayerControl>(*_players[player]);
}

const Motion* GameWorld::motionComponentOf(net::PlayerId player) const {
    if (player >= kMaxPlayers || !_players[player]) {
        return nullptr;
    }
    return _registry.get<Motion>(*_players[player]);
}

} // namespace odyssee::server
