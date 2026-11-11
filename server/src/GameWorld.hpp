#pragma once

#include "InputQueue.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace odyssee::server {

/// L'état de vérité du monde, côté serveur.
///
/// Pour l'instant (T026) : les joueurs connectés, leur orientation (tirée de leurs inputs) et
/// leur position, fixe au point d'apparition. Le mouvement viendra de la simulation partagée
/// (T029) ; le point d'apparition, de la carte chargée par le serveur (T041).
class GameWorld {
  public:
    /// Fait correspondre les joueurs du monde à ceux qui sont connectés : ajoute les nouveaux
    /// (au point d'apparition), retire ceux qui sont partis.
    void syncPlayers(std::span<const net::PlayerId> connected);

    /// Inputs reçus d'un joueur (ignorés s'il n'est pas dans le monde).
    void receiveInputs(net::PlayerId player, const net::InputBatch& batch);

    /// Avance d'un tick : applique au plus un input par joueur.
    void step();

    /// Ce que voit `player` : tous les joueurs, et son dernier input appliqué.
    [[nodiscard]] net::Snapshot snapshotFor(net::PlayerId player, net::Tick serverTick) const;

    [[nodiscard]] bool hasPlayer(net::PlayerId player) const;
    [[nodiscard]] std::size_t playerCount() const;

  private:
    struct PlayerState {
        net::EntityState entity;
        net::Tick lastProcessedInput = 0;
        InputQueue inputs;
    };

    static PlayerState spawn(net::PlayerId player);
    static void apply(PlayerState& state, const InputQueue::Entry& input);

    std::array<std::optional<PlayerState>, kMaxPlayers> _players;
};

} // namespace odyssee::server
