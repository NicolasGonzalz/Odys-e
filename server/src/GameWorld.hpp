#pragma once

#include "ConnectedPlayer.hpp"
#include "InputQueue.hpp"
#include "Crafting.hpp"
#include "Inventory.hpp"
#include "PlayerComponents.hpp"
#include "Vitals.hpp"
#include "ecs/Registry.hpp"
#include "resources/ResourceField.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace odyssee::server {

/// L'état de vérité du monde, côté serveur.
///
/// Où apparaissent les joueurs : en 1/256 m, et cap quantifié (net::quantizeYaw).
struct SpawnPoint {
    std::array<std::int32_t, 3> position{};
    std::uint16_t yaw = 0;
};

/// Les entités du monde vivent dans un ECS (ecs::Registry, T043) ; pour l'instant, ce sont les
/// joueurs connectés (composants PlayerControl et Motion), déplacés par la simulation partagée
/// (sim::step, la même que la prédiction du client) sur le sol et les murs de la carte (T041).
class GameWorld {
  public:
    /// Sans carte : sol plat provisoire (sim/ProvisionalGround.hpp), apparition dans la salle des
    /// capsules du centre de stase.
    GameWorld();
    /// Sur une carte : son sol et ses murs (le client doit charger la même), et son point
    /// d'apparition.
    GameWorld(std::unique_ptr<const sim::Terrain> terrain, SpawnPoint spawn,
              std::uint64_t worldFingerprint = 0);

    /// Empreinte de la carte (world::fingerprintOf), annoncée aux joueurs ; 0 sans carte.
    [[nodiscard]] std::uint64_t worldFingerprint() const { return _worldFingerprint; }

    /// Fait correspondre les joueurs du monde à ceux qui sont connectés : ajoute les nouveaux
    /// (au point d'apparition), retire ceux qui sont partis. Une place dont la génération a
    /// changé est occupée par une autre connexion : le joueur réapparaît, sans rien garder de
    /// l'ancien (position, inputs en attente, dernier input appliqué).
    void syncPlayers(std::span<const ConnectedPlayer> connected);

    /// Inputs reçus d'un joueur (ignorés s'il n'est pas dans le monde).
    void receiveInputs(net::PlayerId player, const net::InputBatch& batch);

    /// Objets et recettes du jeu (data/, T046). Sans catalogue, aucun objet n'existe.
    void useCatalog(data::Catalog catalog) { _catalog = std::move(catalog); }
    [[nodiscard]] const data::Catalog& catalog() const { return _catalog; }

    /// Demande fiable d'un joueur (déplacer un objet de l'inventaire…), vérifiée par le serveur.
    void receiveRequest(net::PlayerId player, const net::GameEvent& request);

    /// Donne des objets à un joueur (récolte, artisanat…). Renvoie ce qui n'a pas trouvé de place.
    std::uint16_t giveItem(net::PlayerId player, data::ItemId item, std::uint16_t count);

    /// L'inventaire d'un joueur s'il a changé depuis le dernier appel (à lui renvoyer), sinon rien.
    [[nodiscard]] std::optional<net::InventoryContents> takeInventoryUpdate(net::PlayerId player);

    /// Les autres événements à envoyer à un joueur (réponses d'artisanat…), depuis le dernier appel.
    [[nodiscard]] std::vector<net::GameEvent> takeEvents(net::PlayerId player);

    /// Inventaire d'un joueur (tests, outils), s'il est dans le monde.
    [[nodiscard]] const Inventory* inventoryOf(net::PlayerId player) const;

    /// Inputs manquants devinés d'affilée, au plus (200 ms). Voir step().
    static constexpr std::uint32_t kMaxGuessedInputs = 4;

    /// À partir de ce nombre d'inputs en attente, le joueur en rattrape un par tick (voir step()).
    static constexpr std::size_t kCatchUpBacklog = 3;

    /// Avance d'un tick : **chaque** joueur est simulé, qu'un input soit arrivé ou non (sinon
    /// un client qui se tait resterait suspendu en l'air, à l'abri de la gravité).
    ///
    /// - Un input en attente : il est appliqué. S'il en reste encore kCatchUpBacklog ou plus
    ///   (retard accumulé pendant une coupure), un second est appliqué dans le même tick : le
    ///   serveur rattrape le client au lieu de garder ce retard pour toujours.
    /// - Aucun, mais le joueur en a déjà envoyé : le tick manquant est **deviné** en rejouant le
    ///   dernier input, et compté comme traité (lastProcessedInput avance). S'il arrive plus
    ///   tard, il est ignoré : le serveur reste aligné sur les ticks du client, qui corrige sa
    ///   prédiction si l'input deviné n'était pas le bon.
    /// - Au-delà de kMaxGuessedInputs d'affilée, ou avant le tout premier input : input neutre
    ///   (aucune touche, même orientation), sans avancer les ticks du client.
    void step();

    /// Ressources récoltables (T044) : `types` (data/resources.txt), placées autour du point
    /// d'apparition par un tirage déterministe (resources/ResourceScatter.hpp). Renvoie le nombre
    /// de ressources créées. Une seule fois, au démarrage.
    std::size_t addResources(data::ResourceCatalog types, const ScatterSettings& settings);
    /// Même chose à des places données (tests ; plus tard, les marqueurs de la carte).
    std::size_t addResources(data::ResourceCatalog types, std::span<const PlacedResource> placed);

    [[nodiscard]] const ResourceField& resources() const { return _resources; }
    /// État d'une ressource par son numéro réseau (tests, outils), si elle existe.
    [[nodiscard]] std::optional<ResourceNode> resourceNode(net::EntityId id) const;

    /// Ce que voit `player` : tous les joueurs, puis les ressources proches de lui (dans la limite
    /// de kMaxSnapshotEntities), son dernier input appliqué et ses besoins vitaux.
    [[nodiscard]] net::Snapshot snapshotFor(net::PlayerId player, net::Tick serverTick) const;

    /// État de mouvement complet d'un joueur (tests, outils), s'il est dans le monde.
    [[nodiscard]] std::optional<sim::PlayerState> motionOf(net::PlayerId player) const;

    /// Besoins vitaux d'un joueur (T051), s'il est dans le monde.
    [[nodiscard]] std::optional<Vitals> vitalsOf(net::PlayerId player) const;

    [[nodiscard]] bool hasPlayer(net::PlayerId player) const;
    [[nodiscard]] std::size_t playerCount() const;

  private:
    void stepPlayer(PlayerControl& control, Motion& motion) const;
    void catchUp(PlayerControl& control, Motion& motion) const;
    void apply(PlayerControl& control, Motion& motion, const net::InputCommand& command,
               net::Tick tick) const;
    void useHeldItem(PlayerControl& control, Vitals& vitals, Inventory& inventory);
    void craftFor(const PlayerControl& control, Crafting& crafting, Inventory& inventory);
    void harvestWith(const PlayerControl& control, const Motion& motion, Harvester& harvester);
    void spawn(const ConnectedPlayer& player);
    void despawn(net::PlayerId player);
    [[nodiscard]] const PlayerControl* controlOf(net::PlayerId player) const;
    [[nodiscard]] const Motion* motionComponentOf(net::PlayerId player) const;
    [[nodiscard]] Inventory* mutableInventoryOf(net::PlayerId player);

    std::unique_ptr<const sim::Terrain> _terrain;
    SpawnPoint _spawn;
    std::uint64_t _worldFingerprint = 0;
    data::Catalog _catalog;
    /// Événements en attente d'envoi, par joueur (voir takeEvents).
    std::array<std::vector<net::GameEvent>, kMaxPlayers> _outbox;
    ecs::Registry _registry;
    /// L'entité de chaque joueur connecté, par numéro de joueur.
    std::array<std::optional<ecs::Entity>, kMaxPlayers> _players;
    ResourceField _resources; ///< arbres, rochers… (T044)
};

} // namespace odyssee::server
