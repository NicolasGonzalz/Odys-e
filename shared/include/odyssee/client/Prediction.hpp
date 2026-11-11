#pragma once

#include "odyssee/client/InputHistory.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace odyssee::client {

/// Prédiction du joueur local (T031) et sa réconciliation avec le serveur (T032).
///
/// Le joueur bouge **tout de suite**, sans attendre le serveur, grâce à la même simulation
/// (sim::step) sur le même sol. Elle part de l'état que le serveur donne pour ce joueur dans un
/// snapshot, puis applique un input par tick du client (ticks numérotés à partir de 1, comme les
/// attend le serveur).
///
/// À chaque snapshot, reconcile() la recale sur la vérité du serveur : elle repart de l'état
/// qu'il a calculé après le dernier input traité, et rejoue par-dessus les inputs qu'il n'a pas
/// encore vus. L'écart corrigé n'est pas montré d'un coup : il fond en quelques ticks
/// (displayPosition), sauf s'il est trop grand (on recale alors franchement).
class Prediction {
  public:
    /// Au-delà de cet écart (2 m, en 1/256 m), on recale d'un coup au lieu de lisser.
    static constexpr std::int32_t kSnapDistance = 512;

    /// `terrain` doit vivre plus longtemps que la prédiction.
    explicit Prediction(const sim::Terrain& terrain) : _terrain(terrain) {}

    /// Démarre sur l'état du joueur `self` dans le snapshot. Faux si le snapshot ne le contient
    /// pas ; sans effet si la prédiction a déjà démarré.
    bool start(const net::Snapshot& snapshot, net::EntityId self);

    [[nodiscard]] bool started() const { return _state.has_value(); }

    /// Joue un tick : l'input est appliqué tout de suite et gardé pour l'envoi. Faux (et sans
    /// effet) tant que la prédiction n'a pas démarré.
    bool advance(const net::InputCommand& command);

    /// Recale sur le snapshot : état du serveur après son dernier input traité, puis rejeu des
    /// inputs suivants. Faux si le snapshot n'apporte rien (pas démarré, aucun input traité,
    /// snapshot plus ancien qu'un déjà utilisé, ou joueur absent).
    bool reconcile(const net::Snapshot& snapshot, net::EntityId self);

    /// État prédit du joueur (absent avant le démarrage).
    [[nodiscard]] const std::optional<sim::PlayerState>& state() const { return _state; }

    /// Position à afficher (1/256 m) : la prédite, plus ce qui reste de l'écart en train de fondre.
    [[nodiscard]] std::array<std::int32_t, 3> displayPosition() const;

    /// Les inputs joués, à envoyer au serveur.
    [[nodiscard]] const InputHistory& history() const { return _history; }

    /// Corrections réelles (écart non nul) depuis le début, et la taille de la dernière (1/256 m).
    [[nodiscard]] std::uint32_t corrections() const { return _corrections; }
    [[nodiscard]] std::int32_t lastCorrection() const { return _lastCorrection; }

  private:
    [[nodiscard]] sim::PlayerState replayFrom(sim::PlayerState state, net::Tick tick) const;
    void applyCorrection(const sim::PlayerState& corrected);
    void fadeDisplayError();

    const sim::Terrain& _terrain;
    std::optional<sim::PlayerState> _state;
    net::Tick _nextTick = 1;       ///< 0 veut dire « aucun input » pour le serveur
    net::Tick _lastReconciled = 0; ///< dernier input traité par le serveur, déjà pris en compte
    InputHistory _history;
    std::array<std::int32_t, 3> _displayError{}; ///< écart encore affiché, fond à chaque tick
    std::uint32_t _corrections = 0;
    std::int32_t _lastCorrection = 0;
};

} // namespace odyssee::client
