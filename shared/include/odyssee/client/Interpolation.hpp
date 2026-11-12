#pragma once

#include "odyssee/net/GameMessages.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

namespace odyssee::client {

/// Une entité distante telle qu'on l'affiche : entre deux snapshots, en position lissée.
struct ShownEntity {
    net::EntityId id = 0;
    std::array<double, 3> position{}; ///< 1/256 m (comme les snapshots), x est, y sud, z haut
    double yawDegrees = 0.0;
};

/// Interpolation des entités distantes (T033) : les autres joueurs bougent sans saccades malgré
/// la latence, la gigue et les pertes.
///
/// On n'affiche pas le dernier snapshot reçu, mais un instant **un peu dans le passé**
/// (kDelayTicks derrière le plus récent) : il y a alors presque toujours deux snapshots qui
/// l'encadrent, entre lesquels on interpole. Un snapshot perdu ou en retard ne fait qu'allonger
/// l'intervalle. L'horloge d'affichage avance au rythme du temps local et se recale doucement sur
/// les snapshots qui arrivent (jamais de saut, sauf très gros décalage).
class Interpolation {
  public:
    /// Retard d'affichage : 3 ticks = 150 ms (absorbe un snapshot perdu et la gigue).
    static constexpr double kDelayTicks = 3.0;
    /// Au-delà de cet écart avec la cible, on recale d'un coup (démarrage, longue coupure).
    static constexpr double kSnapTicks = 20.0;
    static constexpr std::size_t kCapacity = 32; ///< snapshots gardés (1,6 s)

    /// Un snapshot reçu. Plus ancien que l'affichage, ou déjà connu : ignoré.
    void push(const net::Snapshot& snapshot);

    /// Le temps local a avancé de `ticks` (fraction possible : appelé à chaque image).
    void advance(double ticks);

    /// Les entités à l'instant affiché (sauf `except`, le joueur local, prédit à part).
    [[nodiscard]] std::vector<ShownEntity> sample(std::optional<net::EntityId> except = {}) const;

    /// Instant affiché, en ticks du serveur (absent tant qu'aucun snapshot n'est arrivé).
    [[nodiscard]] std::optional<double> renderTick() const { return _renderTick; }

  private:
    struct Frame {
        net::Tick tick = 0;
        std::vector<net::EntityState> entities;
    };

    [[nodiscard]] double targetTick() const;
    void forgetOldFrames();
    [[nodiscard]] static ShownEntity blend(const net::EntityState& a, const net::EntityState* b,
                                          double t);

    std::deque<Frame> _frames; ///< triés par tick croissant
    std::optional<double> _renderTick;
};

} // namespace odyssee::client
