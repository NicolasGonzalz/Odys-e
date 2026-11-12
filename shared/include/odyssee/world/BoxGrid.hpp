#pragma once

#include "odyssee/world/WorldFile.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace odyssee::world {

/// Range les volumes de collision dans une grille de cases de 16 m : pour savoir ce qui touche
/// un point, on ne regarde que les volumes de sa case, pas ceux de toute la carte.
class BoxGrid {
  public:
    static constexpr std::int64_t kCellSize = 16 * 256; ///< 16 m, en 1/256 m

    /// `clearance` : élargissement des volumes (voir covers()), pris en compte dans le rangement.
    BoxGrid(std::span<const CollisionBox> boxes, std::int64_t width, std::int64_t depth,
            std::int32_t clearance);

    /// Indices (dans `boxes`) des volumes qui peuvent toucher le point (x, y). Vide hors carte.
    [[nodiscard]] std::span<const std::uint32_t> near(std::int32_t x, std::int32_t y) const;

  private:
    void insert(std::uint32_t index, const CollisionBox& box, std::int32_t clearance);
    [[nodiscard]] std::int64_t cellColumn(std::int64_t x) const;
    [[nodiscard]] std::int64_t cellRow(std::int64_t y) const;

    std::int64_t _columns = 0;
    std::int64_t _rows = 0;
    std::vector<std::vector<std::uint32_t>> _cells; ///< ligne par ligne
};

} // namespace odyssee::world
