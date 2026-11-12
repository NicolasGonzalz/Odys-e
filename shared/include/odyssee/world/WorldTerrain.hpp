#pragma once

#include "odyssee/sim/Terrain.hpp"
#include "odyssee/world/BoxGrid.hpp"
#include "odyssee/world/WorldFile.hpp"

#include <cstdint>
#include <optional>

namespace odyssee::world {

/// Le sol et les murs de la vraie carte (T041), pour la simulation : relief, cases percées et
/// volumes de collision du fichier monde. Le serveur et la prédiction du client utilisent le
/// même, chargé du même fichier, pour voir exactement le même sol.
///
/// Sol : le plus haut entre la surface du relief (sauf case percée) et le dessus des volumes,
/// sous une hauteur donnée : sous le relief, le joueur marche sur le sol du bunker ou de la
/// grotte, pas sur le relief au-dessus de sa tête. Les bords de la carte sont des murs.
class WorldTerrain final : public sim::Terrain {
  public:
    /// Rayon du joueur (35 cm) : les volumes sont élargis d'autant, on teste son centre.
    static constexpr std::int32_t kPlayerRadius = 90;

    explicit WorldTerrain(WorldFile world);

    [[nodiscard]] std::int32_t groundHeight(std::int32_t x, std::int32_t y) const override;
    [[nodiscard]] std::int32_t groundBelow(std::int32_t x, std::int32_t y,
                                           std::int32_t z) const override;
    [[nodiscard]] bool isSolid(std::int32_t x, std::int32_t y, std::int32_t bottom,
                               std::int32_t top) const override;

    [[nodiscard]] const WorldFile& world() const { return _world; }

  private:
    [[nodiscard]] std::optional<std::int32_t> reliefAt(std::int32_t x, std::int32_t y) const;
    [[nodiscard]] bool isInside(std::int32_t x, std::int32_t y) const;

    WorldFile _world;
    BoxGrid _grid;
};

/// Premier marqueur d'apparition des joueurs de la carte, s'il y en a un.
[[nodiscard]] std::optional<Marker> findPlayerSpawn(const WorldFile& world);

} // namespace odyssee::world
