#pragma once

#include "Inventory.hpp"
#include "death/Death.hpp"
#include "ecs/Registry.hpp"

#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace odyssee::server {

/// Composant : ce que contient un sac laissé à la mort d'un joueur (T052), et à qui il était.
struct BagContents {
    DroppedSlots slots{};
    net::PlayerId owner = 0;

    [[nodiscard]] bool empty() const;
};

/// Les sacs au sol (T052), entités de l'ECS du monde (composants BagContents, Placement et
/// NetworkEntity) : on les pose à la mort, on les ramasse en passant dessus (n'importe quel
/// joueur, comme en coop on s'entraide), ils disparaissent une fois vides.
class DroppedBags {
  public:
    /// Sacs au plus en même temps (numéros réseau à partir de net::kFirstBagEntity).
    static constexpr std::size_t kMaxBags = 256;
    /// Distance horizontale à laquelle on ramasse un sac (1,5 m), et écart de hauteur toléré (2 m).
    static constexpr std::int32_t kPickupRadius = 384;
    static constexpr std::int32_t kPickupHeight = 512;
    /// Rayon autour du joueur dans lequel ses snapshots contiennent les sacs (60 m).
    static constexpr std::int32_t kSnapshotRadius = 60 * 256;

    /// Pose un sac aux pieds `feet`. Renvoie son numéro réseau ; std::nullopt s'il n'y a rien à
    /// laisser ou si le plafond est atteint (le contenu est alors perdu).
    std::optional<net::EntityId> drop(ecs::Registry& registry,
                                      const std::array<std::int32_t, 3>& feet,
                                      const BagContents& contents);

    /// Le joueur aux pieds `feet` ramasse ce qui tient dans son inventaire, dans les sacs à sa
    /// portée. Un sac vidé disparaît. Renvoie vrai si quelque chose a été pris.
    bool pickUp(ecs::Registry& registry, const std::array<std::int32_t, 3>& feet,
                Inventory& inventory, const data::Catalog& catalog);

    /// Ajoute à `entities` les sacs à moins de kSnapshotRadius de `around`, des plus proches aux
    /// plus lointains, sans que `entities` dépasse `limit`.
    void appendNearby(const ecs::Registry& registry, const std::array<std::int32_t, 3>& around,
                      std::vector<net::EntityState>& entities, std::size_t limit) const;

    /// Contenu d'un sac par son numéro réseau (tests, outils, sauvegarde), s'il existe.
    [[nodiscard]] std::optional<BagContents> contentsOf(const ecs::Registry& registry,
                                                        net::EntityId id) const;

    [[nodiscard]] std::size_t size() const { return _bags.size(); }
    [[nodiscard]] std::span<const ecs::Entity> bags() const { return _bags; }

  private:
    [[nodiscard]] std::optional<net::EntityId> freeId(const ecs::Registry& registry) const;
    void forget(ecs::Registry& registry, ecs::Entity bag);

    std::vector<ecs::Entity> _bags;
};

} // namespace odyssee::server
