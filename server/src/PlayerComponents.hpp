#pragma once

#include "ConnectedPlayer.hpp"
#include "InputQueue.hpp"

#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"

#include <cstdint>

namespace odyssee::server {

/// Composants des entités du monde (ecs::Registry) : des données, sans logique.

/// Tout ce qui se déplace par la simulation partagée (joueurs ; créatures plus tard).
struct Motion {
    sim::PlayerState state;
};

/// Une entité pilotée par un joueur connecté : ses inputs reçus et où il en est.
struct PlayerControl {
    net::PlayerId id = 0;
    ConnectionGeneration generation = 0; ///< connexion qui occupe la place (voir syncPlayers)
    net::Tick lastProcessedInput = 0;
    InputQueue inputs;
    net::InputCommand lastCommand;   ///< dernier input appliqué (reçu ou deviné)
    std::uint32_t guessedInARow = 0; ///< inputs devinés depuis le dernier input reçu
    /// Boutons enfoncés pendant ce tick (et pas au précédent) : un appui sur E mange une fois.
    std::uint16_t pressedThisTick = 0;
};

} // namespace odyssee::server
