#include "resources/HarvestReach.hpp"

#include "odyssee/net/Quantize.hpp"

#include <cmath>
#include <cstdlib>

namespace odyssee::server {

namespace {

constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

/// Vrai si la direction (dx, dy), non nulle, est dans le cône devant le cap `yaw`.
bool isInFront(std::int64_t dx, std::int64_t dy, std::uint16_t yaw) {
    // Cap de +x vers +y : (cos, sin) donne la direction du regard dans le repère du monde.
    const double heading = static_cast<double>(net::dequantizeYaw(yaw)) * kDegreesToRadians;
    const double x = static_cast<double>(dx);
    const double y = static_cast<double>(dy);
    const double along = x * std::cos(heading) + y * std::sin(heading);
    const double minAlong =
        std::hypot(x, y) * std::cos(kHarvestConeHalfAngleDegrees * kDegreesToRadians);
    return along >= minAlong;
}

} // namespace

std::int64_t horizontalDistanceSquared(const std::array<std::int32_t, 3>& a,
                                       const std::array<std::int32_t, 3>& b) {
    const std::int64_t dx = std::int64_t{b[0]} - a[0];
    const std::int64_t dy = std::int64_t{b[1]} - a[1];
    return dx * dx + dy * dy;
}

bool isWithinReach(const std::array<std::int32_t, 3>& feet, std::uint16_t yaw,
                   const std::array<std::int32_t, 3>& target) {
    if (std::llabs(std::int64_t{target[2]} - feet[2]) > kHarvestHeight) {
        return false;
    }
    const std::int64_t distanceSquared = horizontalDistanceSquared(feet, target);
    if (distanceSquared > std::int64_t{kHarvestReach} * kHarvestReach) {
        return false;
    }
    if (distanceSquared <= std::int64_t{kPointBlank} * kPointBlank) {
        return true;
    }
    return isInFront(std::int64_t{target[0]} - feet[0], std::int64_t{target[1]} - feet[1], yaw);
}

} // namespace odyssee::server
