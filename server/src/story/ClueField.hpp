#pragma once

#include "ecs/Registry.hpp"

#include "odyssee/data/StoryCatalog.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace odyssee::server {

/// Composant : quel indice est posé là.
struct ClueNode {
    data::ClueId clue;
};

/// Les indices posés dans le monde (T049), entités de l'ECS du monde (composants ClueNode,
/// Placement et NetworkEntity) : on les lit en les visant, comme on récolte.
class ClueField {
  public:
    /// Rayon autour du joueur dans lequel ses snapshots contiennent les indices (40 m).
    static constexpr std::int32_t kSnapshotRadius = 40 * 256;

    /// Pose chaque indice de `story` à sa place (mètres depuis `spawn`), au sol. Renvoie le nombre
    /// d'indices posés. Une seule fois, au démarrage.
    std::size_t place(ecs::Registry& registry, const data::StoryCatalog& story,
                      const sim::Terrain& terrain, const std::array<std::int32_t, 3>& spawn);

    /// L'indice que vise un joueur aux pieds `feet` et de cap `yaw` (le plus proche), s'il y en a.
    [[nodiscard]] std::optional<data::ClueId> target(const ecs::Registry& registry,
                                                     const std::array<std::int32_t, 3>& feet,
                                                     std::uint16_t yaw) const;

    /// Ajoute à `entities` les indices à moins de kSnapshotRadius de `around`, des plus proches aux
    /// plus lointains, sans dépasser `limit`. `read` dit si un indice est déjà dans le journal du
    /// destinataire (drapeau net::kClueRead).
    void appendNearby(const ecs::Registry& registry, const std::array<std::int32_t, 3>& around,
                      const std::function<bool(data::ClueId)>& read,
                      std::vector<net::EntityState>& entities, std::size_t limit) const;

    [[nodiscard]] std::size_t size() const { return _clues.size(); }

  private:
    std::vector<ecs::Entity> _clues;
};

} // namespace odyssee::server
