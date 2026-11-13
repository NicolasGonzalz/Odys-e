#pragma once

#include "ecs/Registry.hpp"
#include "resources/Harvesting.hpp"
#include "resources/ResourceComponents.hpp"
#include "resources/ResourceScatter.hpp"

#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace odyssee::server {

/// Les ressources récoltables du monde (T044), entités de l'ECS du monde (composants
/// ResourceNode, Placement et NetworkEntity) : leur création, leur repousse, les coups des
/// joueurs et ce qui en part dans les snapshots. Les types (data/resources.txt) sont gardés ici.
class ResourceField {
  public:
    /// Ressources au plus (numéros réseau à partir de net::kFirstResourceEntity).
    static constexpr std::size_t kMaxNodes = 4096;
    /// Rayon autour du joueur dans lequel ses snapshots contiennent les ressources (60 m).
    static constexpr std::int32_t kSnapshotRadius = 60 * 256;

    /// Ce qu'un coup a donné : le gain, et les ticks avant le coup suivant.
    struct Strike {
        Harvest harvest;
        std::uint32_t cooldownTicks = 0;
        net::EntityId node = 0;
    };

    /// Les types connus (remplace les précédents ; à faire avant spawn).
    void setTypes(data::ResourceCatalog types) { _types = std::move(types); }
    [[nodiscard]] const data::ResourceCatalog* types() const { return _types ? &*_types : nullptr; }

    /// Crée une entité pleine par ressource placée (type inconnu ou plafond atteint : ignorée).
    /// Renvoie le nombre d'entités créées.
    std::size_t spawn(ecs::Registry& registry, std::span<const PlacedResource> placed);

    /// Système « repousse » : un tick pour chaque ressource épuisée.
    void stepRegrowth(ecs::Registry& registry) const;

    /// Un coup du joueur (pieds en `feet`, cap `yaw`) avec `tool` sur la plus proche des
    /// ressources récoltables qu'il vise (HarvestReach.hpp). std::nullopt s'il n'y en a pas.
    [[nodiscard]] std::optional<Strike> strike(ecs::Registry& registry,
                                               const std::array<std::int32_t, 3>& feet,
                                               std::uint16_t yaw, HeldTool tool) const;

    /// Ajoute à `entities` les ressources à moins de kSnapshotRadius de `around`, des plus
    /// proches aux plus lointaines, sans que `entities` dépasse `limit`.
    void appendNearby(const ecs::Registry& registry, const std::array<std::int32_t, 3>& around,
                      std::vector<net::EntityState>& entities, std::size_t limit) const;

    [[nodiscard]] std::size_t size() const { return _nodes.size(); }
    /// Les entités des ressources, dans l'ordre de création.
    [[nodiscard]] std::span<const ecs::Entity> nodes() const { return _nodes; }

  private:
    [[nodiscard]] std::optional<ecs::Entity> findTarget(const ecs::Registry& registry,
                                                        const std::array<std::int32_t, 3>& feet,
                                                        std::uint16_t yaw, HeldTool tool) const;
    [[nodiscard]] bool canStrike(const ecs::Registry& registry, ecs::Entity entity,
                                 HeldTool tool) const;

    std::optional<data::ResourceCatalog> _types;
    std::vector<ecs::Entity> _nodes;
};

/// État réseau d'une ressource : type et épuisement dans les drapeaux (net::kResourceTypeMask).
[[nodiscard]] net::EntityState toResourceEntity(const NetworkEntity& network,
                                                const Placement& placement,
                                                const ResourceNode& node);

} // namespace odyssee::server
