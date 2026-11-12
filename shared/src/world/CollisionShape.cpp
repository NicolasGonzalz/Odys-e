#include "odyssee/world/CollisionShape.hpp"

#include "odyssee/sim/FixedMath.hpp"

#include <cstdlib>

namespace odyssee::world {

namespace {

struct LocalPoint {
    std::int64_t x = 0;
    std::int64_t y = 0;
};

/// (x, y) dans le repère du volume : relatif à son centre, puis tourné de -cap.
LocalPoint toLocal(const CollisionBox& box, std::int32_t x, std::int32_t y) {
    const std::int64_t dx = std::int64_t{x} - box.center[0];
    const std::int64_t dy = std::int64_t{y} - box.center[1];
    const std::int64_t cosine = sim::cosQ15(box.yaw);
    const std::int64_t sine = sim::sinQ15(box.yaw);
    return {(dx * cosine + dy * sine) / sim::kQ15One, (dy * cosine - dx * sine) / sim::kQ15One};
}

/// Dans l'ellipse de demi-axes (a, b), le point étant déjà dans le rectangle qui l'entoure :
/// (x/a)² + (y/b)² ≤ 1, calculé en Q15 pour ne pas déborder sur 64 bits.
bool insideEllipse(LocalPoint p, std::int64_t a, std::int64_t b) {
    const std::int64_t nx = std::llabs(p.x) * sim::kQ15One / a;
    const std::int64_t ny = std::llabs(p.y) * sim::kQ15One / b;
    return nx * nx + ny * ny <= std::int64_t{sim::kQ15One} * sim::kQ15One;
}

} // namespace

bool covers(const CollisionBox& box, std::int32_t x, std::int32_t y, std::int32_t clearance) {
    const LocalPoint p = toLocal(box, x, y);
    const std::int64_t a = std::int64_t{box.halfExtents[0]} + clearance;
    const std::int64_t b = std::int64_t{box.halfExtents[1]} + clearance;
    if (std::llabs(p.x) > a || std::llabs(p.y) > b) {
        return false;
    }
    return box.shape == BoxShape::Box || insideEllipse(p, a, b);
}

std::int32_t bottomOf(const CollisionBox& box) {
    return static_cast<std::int32_t>(std::int64_t{box.center[2]} - box.halfExtents[2]);
}

std::int32_t topOf(const CollisionBox& box) {
    return static_cast<std::int32_t>(std::int64_t{box.center[2]} + box.halfExtents[2]);
}

std::int64_t footprintRadius(const CollisionBox& box, std::int32_t clearance) {
    // Demi-diagonale majorée par la somme des demi-côtés : large, mais sans racine carrée.
    return std::int64_t{box.halfExtents[0]} + box.halfExtents[1] + 2 * std::int64_t{clearance} + 1;
}

} // namespace odyssee::world
