#pragma once

#include "Hud.hpp"
#include "InputSampler.hpp"

#include "odyssee/client/Interpolation.hpp"
#include "odyssee/client/Prediction.hpp"
#include "odyssee/client/ServerLink.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

namespace odyssee::debugclient {

/// Le client de debug : il se connecte au serveur, prédit le joueur local (T031) en se recalant
/// sur chaque snapshot (T032), et affiche les autres joueurs interpolés entre deux snapshots,
/// avec un léger retard qui absorbe latence, gigue et pertes (T033).
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
    void reconcileWithLatest();
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
    client::Interpolation _remote; ///< les autres joueurs, affichés entre deux snapshots (T033)
    /// Position affichée au tick d'avant (rendu fluide entre deux ticks), en 1/256 m.
    std::optional<std::array<std::int32_t, 3>> _previousDisplay;
    bool _thirdPerson = false;
};

} // namespace odyssee::debugclient
