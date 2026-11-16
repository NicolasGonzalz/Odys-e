#include "ClueShapes.hpp"

#include "Scene.hpp"

#include <raylib.h>

namespace odyssee::debugclient {

namespace {

constexpr Color kStand{90, 70, 50, 255};
constexpr Color kUnread{250, 215, 60, 255};
constexpr Color kRead{170, 170, 160, 255};

void drawClue(Vector3 base, bool read) {
    DrawCube({base.x, base.y + 0.45f, base.z}, 0.08f, 0.9f, 0.08f, kStand);
    const Vector3 sheet{base.x, base.y + 0.95f, base.z};
    DrawCube(sheet, 0.4f, 0.05f, 0.3f, read ? kRead : kUnread);
    DrawCubeWires(sheet, 0.4f, 0.05f, 0.3f, BLACK);
}

} // namespace

void drawClues(std::span<const net::EntityState> entities) {
    for (const net::EntityState& entity : entities) {
        if (entity.kind == net::EntityKind::Clue) {
            drawClue(toWorld(entity.position), (entity.flags & net::kClueRead) != 0);
        }
    }
}

} // namespace odyssee::debugclient
