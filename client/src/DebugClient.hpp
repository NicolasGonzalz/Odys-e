#pragma once

#include "Hud.hpp"
#include "InputSampler.hpp"

#include "odyssee/client/Prediction.hpp"
#include "odyssee/client/ServerLink.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <chrono>
#include <optional>

namespace odyssee::debugclient {

/// Le client de debug : il se connecte au serveur, prédit le joueur local (T031) et affiche
/// les autres joueurs tels que les décrit le dernier snapshot (l'interpolation viendra en T033).
///
/// La logique tourne à 20 ticks par seconde comme le serveur ; l'affichage, à chaque image,
/// place le joueur entre ses deux derniers ticks pour que la caméra ne saccade pas.
class DebugClient {
  public:
    using Clock = std::chrono::steady_clock;

    explicit DebugClient(client::ServerLink link);

    // La prédiction garde une référence vers le sol : l'objet ne se copie ni ne se déplace.
    DebugClient(const DebugClient&) = delete;
    DebugClient& operator=(const DebugClient&) = delete;

    /// Boucle principale, jusqu'à la fermeture de la fenêtre (déjà ouverte).
    void run();

  private:
    void handleWindowKeys();
    void tick(Clock::time_point now);
    void keepLatestSnapshot();
    void startPredictionWhenPossible();
    void draw(float betweenTicks) const;
    void drawWorld(float betweenTicks) const;
    [[nodiscard]] HudInfo hudInfo() const;
    void leave();

    client::ServerLink _link;
    sim::FlatTerrain _ground;
    client::Prediction _prediction; ///< après _ground : elle s'y réfère
    InputSampler _input;
    std::optional<net::Snapshot> _latest;
    std::optional<sim::PlayerState> _previous; ///< état prédit au tick d'avant (rendu fluide)
    bool _thirdPerson = false;
};

} // namespace odyssee::debugclient
