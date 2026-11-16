#include "StationShapes.hpp"

#include "Scene.hpp"

#include <raylib.h>

namespace odyssee::debugclient {

namespace {

constexpr Color kWood{120, 80, 45, 255};
constexpr Color kMapPaper{235, 225, 190, 255};

void drawMapTable(Vector3 base) {
    for (const float dx : {-0.45f, 0.45f}) {
        for (const float dz : {-0.3f, 0.3f}) {
            DrawCube({base.x + dx, base.y + 0.4f, base.z + dz}, 0.08f, 0.8f, 0.08f, kWood);
        }
    }
    const Vector3 top{base.x, base.y + 0.82f, base.z};
    DrawCube(top, 1.1f, 0.06f, 0.8f, kWood);
    DrawCube({top.x, top.y + 0.04f, top.z}, 0.8f, 0.01f, 0.55f, kMapPaper);
    DrawCubeWires(top, 1.1f, 0.06f, 0.8f, BLACK);
}

} // namespace

void drawStations(std::span<const net::EntityState> entities) {
    for (const net::EntityState& entity : entities) {
        if (entity.kind == net::EntityKind::Station &&
            entity.flags == net::kStationCartographyTable) {
            drawMapTable(toWorld(entity.position));
        }
    }
}

} // namespace odyssee::debugclient
