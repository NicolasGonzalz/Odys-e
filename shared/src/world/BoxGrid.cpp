#include "odyssee/world/BoxGrid.hpp"

#include "odyssee/world/CollisionShape.hpp"

#include <algorithm>

namespace odyssee::world {

BoxGrid::BoxGrid(std::span<const CollisionBox> boxes, std::int64_t width, std::int64_t depth,
                 std::int32_t clearance)
    : _columns(width / kCellSize + 1), _rows(depth / kCellSize + 1),
      _cells(static_cast<std::size_t>(_columns * _rows)) {
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        insert(static_cast<std::uint32_t>(i), boxes[i], clearance);
    }
}

std::span<const std::uint32_t> BoxGrid::near(std::int32_t x, std::int32_t y) const {
    if (x < 0 || y < 0 || x / kCellSize >= _columns || y / kCellSize >= _rows) {
        return {};
    }
    return _cells[static_cast<std::size_t>((y / kCellSize) * _columns + x / kCellSize)];
}

void BoxGrid::insert(std::uint32_t index, const CollisionBox& box, std::int32_t clearance) {
    // Toutes les cases que touche le carré qui entoure l'empreinte, quelle que soit la rotation.
    const std::int64_t radius = footprintRadius(box, clearance);
    const std::int64_t firstColumn = cellColumn(box.center[0] - radius);
    const std::int64_t lastColumn = cellColumn(box.center[0] + radius);
    const std::int64_t firstRow = cellRow(box.center[1] - radius);
    const std::int64_t lastRow = cellRow(box.center[1] + radius);

    for (std::int64_t row = firstRow; row <= lastRow; ++row) {
        for (std::int64_t column = firstColumn; column <= lastColumn; ++column) {
            _cells[static_cast<std::size_t>(row * _columns + column)].push_back(index);
        }
    }
}

std::int64_t BoxGrid::cellColumn(std::int64_t x) const {
    return std::clamp<std::int64_t>(x, 0, (_columns - 1) * kCellSize) / kCellSize;
}

std::int64_t BoxGrid::cellRow(std::int64_t y) const {
    return std::clamp<std::int64_t>(y, 0, (_rows - 1) * kCellSize) / kCellSize;
}

} // namespace odyssee::world
