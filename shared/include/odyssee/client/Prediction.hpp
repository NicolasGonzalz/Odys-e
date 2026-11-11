#pragma once

#include "odyssee/client/InputHistory.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <optional>

namespace odyssee::client {

/// Prédiction du joueur local (T031) : il bouge **tout de suite**, sans attendre la réponse du
/// serveur, grâce à la même simulation (sim::step) sur le même sol.
///
/// Elle part de l'état que le serveur donne pour ce joueur dans un snapshot, puis applique un
/// input par tick du client (ticks numérotés à partir de 1, comme les attend le serveur). Les
/// inputs sont gardés pour l'envoi (en lot) et, plus tard, pour la réconciliation (T032).
class Prediction {
  public:
    /// `terrain` doit vivre plus longtemps que la prédiction.
    explicit Prediction(const sim::Terrain& terrain) : _terrain(terrain) {}

    /// Démarre sur l'état du joueur `self` dans le snapshot. Faux si le snapshot ne le contient
    /// pas ; sans effet si la prédiction a déjà démarré.
    bool start(const net::Snapshot& snapshot, net::EntityId self);

    [[nodiscard]] bool started() const { return _state.has_value(); }

    /// Joue un tick : l'input est appliqué tout de suite et gardé pour l'envoi. Faux (et sans
    /// effet) tant que la prédiction n'a pas démarré.
    bool advance(const net::InputCommand& command);

    /// État prédit du joueur (absent avant le démarrage).
    [[nodiscard]] const std::optional<sim::PlayerState>& state() const { return _state; }

    /// Les inputs joués, à envoyer au serveur.
    [[nodiscard]] const InputHistory& history() const { return _history; }

  private:
    const sim::Terrain& _terrain;
    std::optional<sim::PlayerState> _state;
    net::Tick _nextTick = 1; ///< 0 veut dire « aucun input » pour le serveur
    InputHistory _history;
};

} // namespace odyssee::client
