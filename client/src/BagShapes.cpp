#include "BagShapes.hpp"

#include "Scene.hpp"

#include <raylib.h>

namespace odyssee::debugclient {

namespace {

constexpr Color kCanvas{120, 82, 48, 255};
constexpr Color kKnot{70, 48, 28, 255};

void drawBag(Vector3 base) {
    const Vector3 body{base.x, base.y + 0.25f, base.z};
    DrawCube(body, 0.6f, 0.5f, 0.5f, kCanvas);
    DrawCubeWires(body, 0.6f, 0.5f, 0.5f, BLACK);
    DrawSphere({base.x, base.y + 0.58f, base.z}, 0.12f, kKnot);
}

} // namespace

void drawBags(std::span<const net::EntityState> entities) {
    for (const net::EntityState& entity : entities) {
        if (entity.kind == net::EntityKind::Bag) {
            drawBag(toWorld(entity.position));
        }
    }
}

} // namespace odyssee::debugclient
