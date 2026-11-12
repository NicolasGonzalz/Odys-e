#pragma once

#include "Hud.hpp"
#include "InputSampler.hpp"
#include "MapView.hpp"

#include "odyssee/client/Interpolation.hpp"
#include "odyssee/client/Prediction.hpp"
#include "odyssee/client/ServerLink.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
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

    /// `map` : la carte, la même que celle du serveur (sinon la prédiction diverge). Sans
    /// carte : le sol plat provisoire du serveur lancé sans carte.
    DebugClient(client::ServerLink link, std::unique_ptr<const world::WorldTerrain> map);

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
    std::unique_ptr<const world::WorldTerrain> _map; ///< nul : sol plat provisoire
    sim::FlatTerrain _flat;
    client::Prediction _prediction; ///< après _map et _flat : elle se réfère à l'un d'eux
    std::optional<MapView> _mapView;
    InputSampler _input;
    std::optional<net::Snapshot> _latest;
    client::Interpolation _remote; ///< les autres joueurs, affichés entre deux snapshots (T033)
    /// Position affichée au tick d'avant (rendu fluide entre deux ticks), en 1/256 m.
    std::optional<std::array<std::int32_t, 3>> _previousDisplay;
    bool _thirdPerson = false;
};

} // namespace odyssee::debugclient
