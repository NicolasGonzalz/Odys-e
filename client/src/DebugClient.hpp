#pragma once

#include "Hud.hpp"
#include "InputSampler.hpp"
#include "MapView.hpp"
#include "PauseMenu.hpp"

#include "odyssee/client/Interpolation.hpp"
#include "odyssee/client/Prediction.hpp"
#include "odyssee/client/ServerLink.hpp"
#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"
#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/GameEvents.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace odyssee::debugclient {

/// Ce que le client charge une fois au lancement et garde d'une partie à l'autre.
struct GameData {
    const world::WorldTerrain* map = nullptr; ///< nulle : sol plat provisoire
    const data::Catalog* catalog = nullptr;   ///< noms des objets (inventaire) ; nul : numéros
    /// data/resources.txt, pour dessiner les ressources (nul : formes neutres).
    const data::ResourceCatalog* resourceTypes = nullptr;
};

/// Pourquoi une partie s'est terminée.
struct SessionEnd {
    bool quit = false;   ///< fermer le client ; sinon, retour à l'écran de connexion
    std::string message; ///< à afficher sur l'écran de connexion (refus, coupure…)
};

/// Le client de debug : il se connecte au serveur, prédit le joueur local (T031) en se recalant
/// sur chaque snapshot (T032), et affiche les autres joueurs interpolés entre deux snapshots,
/// avec un léger retard qui absorbe latence, gigue et pertes (T033).
///
/// La logique tourne à 20 ticks par seconde comme le serveur ; l'affichage, à chaque image,
/// place le joueur entre ses deux derniers ticks pour que la caméra ne saccade pas.
class DebugClient {
  public:
    using Clock = std::chrono::steady_clock;

    /// `map` : la carte, la même que celle du serveur (sinon la prédiction diverge), gardée par
    /// l'appelant tant que l'objet vit. Nulle : le sol plat du serveur lancé sans carte.
    DebugClient(client::ServerLink link, GameData data);

    // La prédiction garde une référence vers le sol : l'objet ne se copie ni ne se déplace.
    DebugClient(const DebugClient&) = delete;
    DebugClient& operator=(const DebugClient&) = delete;

    /// Se connecte et joue jusqu'à la fin de la partie : fenêtre fermée, choix du menu Échap
    /// (T056), refus du serveur ou connexion perdue. La fenêtre doit être ouverte.
    [[nodiscard]] SessionEnd run();

  private:
    void advance(float& accumulated);
    [[nodiscard]] std::optional<SessionEnd> endAfter(PauseChoice choice);
    [[nodiscard]] std::optional<SessionEnd> connectionLost() const;
    /// Empreinte de la carte chargée (world::kNoMapFingerprint sans carte).
    [[nodiscard]] std::uint64_t ownMap() const;
    void handleWindowKeys();
    void setPaused(bool paused);
    void tick(Clock::time_point now);
    void keepLatestSnapshot();
    void keepEvents();
    [[nodiscard]] std::string hotbarText() const;
    void reconcileWithLatest();
    void startPredictionWhenPossible();
    [[nodiscard]] PauseChoice draw(float betweenTicks) const;
    void drawWorld(float betweenTicks) const;
    [[nodiscard]] HudInfo hudInfo() const;
    void leave();

    client::ServerLink _link;
    const world::WorldTerrain* _map; ///< nul : sol plat provisoire
    const data::ResourceCatalog* _resourceTypes; ///< nul : ressources en formes neutres
    const data::Catalog* _catalog;   ///< nul : objets montrés par leur numéro
    sim::FlatTerrain _flat;
    client::Prediction _prediction; ///< après _map et _flat : elle se réfère à l'un d'eux
    std::optional<MapView> _mapView;
    InputSampler _input;
    std::optional<net::Snapshot> _latest;
    std::optional<net::InventoryContents> _inventory; ///< dernier reçu du serveur (T045)
    client::Interpolation _remote; ///< les autres joueurs, affichés entre deux snapshots (T033)
    /// Position affichée au tick d'avant (rendu fluide entre deux ticks), en 1/256 m.
    std::optional<std::array<std::int32_t, 3>> _previousDisplay;
    bool _thirdPerson = false;
    bool _paused = false; ///< menu Échap ouvert : la souris est libre, le joueur ne bouge plus
};

} // namespace odyssee::debugclient
