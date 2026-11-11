#pragma once

#include "ConnectedPlayer.hpp"
#include "InputQueue.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace odyssee::server {

/// L'état de vérité du monde, côté serveur.
///
/// Pour l'instant : les joueurs connectés, déplacés par la simulation partagée (sim::step, la
/// même que la prédiction du client) sur un sol plat. La carte, son relief et son point
/// d'apparition viendront avec son chargement par le serveur (T041).
class GameWorld {
  public:
    GameWorld();

    /// Fait correspondre les joueurs du monde à ceux qui sont connectés : ajoute les nouveaux
    /// (au point d'apparition), retire ceux qui sont partis. Une place dont la génération a
    /// changé est occupée par une autre connexion : le joueur réapparaît, sans rien garder de
    /// l'ancien (position, inputs en attente, dernier input appliqué).
    void syncPlayers(std::span<const ConnectedPlayer> connected);

    /// Inputs reçus d'un joueur (ignorés s'il n'est pas dans le monde).
    void receiveInputs(net::PlayerId player, const net::InputBatch& batch);

    /// Avance d'un tick : applique au plus un input par joueur.
    void step();

    /// Ce que voit `player` : tous les joueurs, et son dernier input appliqué.
    [[nodiscard]] net::Snapshot snapshotFor(net::PlayerId player, net::Tick serverTick) const;

    /// État de mouvement complet d'un joueur (tests, outils), s'il est dans le monde.
    [[nodiscard]] std::optional<sim::PlayerState> motionOf(net::PlayerId player) const;

    [[nodiscard]] bool hasPlayer(net::PlayerId player) const;
    [[nodiscard]] std::size_t playerCount() const;

  private:
    struct PlayerState {
        ConnectionGeneration generation = 0;
        sim::PlayerState motion;
        net::Tick lastProcessedInput = 0;
        InputQueue inputs;
    };

    static PlayerState spawn(const ConnectedPlayer& player);

    sim::FlatTerrain _terrain; ///< sol plat à la hauteur du point d'apparition, en attendant T041
    std::array<std::optional<PlayerState>, kMaxPlayers> _players;
};

} // namespace odyssee::server
