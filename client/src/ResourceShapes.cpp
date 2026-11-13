#include "ResourceShapes.hpp"

#include "Scene.hpp"

#include <raylib.h>

#include <optional>

namespace odyssee::debugclient {

namespace {

/// Une ressource épuisée : couleur délavée vers le blanc.
Color faded(Color color) {
    const auto toward = [](unsigned char c) {
        return static_cast<unsigned char>((static_cast<int>(c) + 2 * 235) / 3);
    };
    return Color{toward(color.r), toward(color.g), toward(color.b), color.a};
}

void drawTree(Vector3 base, float scale, Color trunk) {
    DrawCylinder(base, 0.3f * scale, 0.35f * scale, 4.0f * scale, 10, trunk);
    DrawCylinderWires(base, 0.3f * scale, 0.35f * scale, 4.0f * scale, 10, BLACK);
}

void drawBlock(Vector3 base, Vector3 size, Color color) {
    const Vector3 center{base.x, base.y + size.y / 2, base.z};
    DrawCube(center, size.x, size.y, size.z, color);
    DrawCubeWires(center, size.x, size.y, size.z, BLACK);
}

void drawBush(Vector3 base, float scale, Color color) {
    DrawSphere({base.x, base.y + 0.4f * scale, base.z}, 0.6f * scale, color);
}

void drawShape(Vector3 base, std::optional<data::ResourceShape> shape, bool depleted) {
    const float scale = depleted ? 0.6f : 1.0f;
    const auto tint = [depleted](Color color) { return depleted ? faded(color) : color; };
    if (!shape) {
        drawBlock(base, {0.8f, 0.8f, 0.8f}, tint(PURPLE));
        return;
    }
    switch (*shape) {
    case data::ResourceShape::Tree:
        drawTree(base, scale, tint(Color{110, 72, 40, 255}));
        return;
    case data::ResourceShape::Rock:
        drawBlock(base, {1.4f * scale, 1.0f * scale, 1.2f * scale}, tint(GRAY));
        return;
    case data::ResourceShape::Bush:
        drawBush(base, scale, tint(Color{70, 140, 60, 255}));
        return;
    case data::ResourceShape::Vein:
        drawBlock(base, {1.2f * scale, 0.9f * scale, 1.2f * scale}, tint(Color{70, 60, 60, 255}));
        drawBlock(base, {1.25f * scale, 0.3f * scale, 0.3f * scale}, tint(ORANGE));
        return;
    }
}

std::optional<data::ResourceShape> shapeOf(const net::EntityState& entity,
                                           const data::ResourceCatalog* types) {
    const data::ResourceTypeId id{static_cast<std::uint8_t>(entity.flags & net::kResourceTypeMask)};
    const data::ResourceDef* type = types ? types->type(id) : nullptr;
    return type ? std::optional<data::ResourceShape>(type->shape) : std::nullopt;
}

} // namespace

void drawResources(std::span<const net::EntityState> entities, const data::ResourceCatalog* types) {
    for (const net::EntityState& entity : entities) {
        if (entity.kind != net::EntityKind::Resource) {
            continue;
        }
        const bool depleted = (entity.flags & net::kResourceDepleted) != 0;
        drawShape(toWorld(entity.position), shapeOf(entity, types), depleted);
    }
}

} // namespace odyssee::debugclient
