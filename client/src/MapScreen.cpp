#include "MapScreen.hpp"

#include "odyssee/net/Quantize.hpp"

#include <raylib.h>

#include <algorithm>
#include <string>

namespace odyssee::debugclient {

namespace {

constexpr Color kPaper{240, 236, 222, 255};
constexpr Color kInk{40, 35, 30, 255};
constexpr Color kPending{250, 225, 140, 255};
constexpr Color kTable{120, 80, 45, 255};
constexpr int kMargin = 2; ///< carrés vides autour de ce qui est connu
constexpr float kChunkUnits = static_cast<float>(net::kMapChunkMeters * 256);

/// Les carrés montrés : tout ce qui est connu, plus une marge.
struct Bounds {
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
};

Bounds boundsOf(const MapState& map) {
    Bounds bounds{1 << 15, 1 << 15, -(1 << 15), -(1 << 15)};
    for (const auto* layer : {&map.shared(), &map.pending()}) {
        for (const net::MapChunk& chunk : *layer) {
            bounds = {std::min(bounds.minX, int{chunk.x}), std::min(bounds.minY, int{chunk.y}),
                      std::max(bounds.maxX, int{chunk.x}), std::max(bounds.maxY, int{chunk.y})};
        }
    }
    if (bounds.minX > bounds.maxX) {
        return {};
    }
    return {bounds.minX - kMargin, bounds.minY - kMargin, bounds.maxX + kMargin,
            bounds.maxY + kMargin};
}

/// Couleur d'un carré connu, d'après la hauteur du relief en son centre (plaine verte → roche).
Color chunkColor(net::MapChunk chunk, const world::WorldTerrain* terrain) {
    if (!terrain) {
        return Color{120, 165, 95, 255};
    }
    const auto center = [](std::int16_t c) {
        return static_cast<std::int32_t>((static_cast<float>(c) + 0.5f) * kChunkUnits);
    };
    const float height =
        net::dequantizePosition(terrain->groundHeight(center(chunk.x), center(chunk.y)));
    const float t = std::clamp(height / 120.0f, 0.0f, 1.0f);
    const auto mix = [t](float low, float high) {
        return static_cast<unsigned char>(low + (high - low) * t);
    };
    return Color{mix(110, 150), mix(160, 140), mix(90, 120), 255};
}

struct Layout {
    Bounds bounds;
    float cell = 1.0f;
    float left = 0.0f, top = 0.0f;

    [[nodiscard]] Rectangle cellOf(net::MapChunk chunk) const {
        return {left + static_cast<float>(chunk.x - bounds.minX) * cell,
                top + static_cast<float>(chunk.y - bounds.minY) * cell, cell, cell};
    }
    /// Position à l'écran d'un point du monde (1/256 m).
    [[nodiscard]] Vector2 pointOf(const std::array<std::int32_t, 3>& position) const {
        return {left + (static_cast<float>(position[0]) / kChunkUnits -
                        static_cast<float>(bounds.minX)) *
                           cell,
                top + (static_cast<float>(position[1]) / kChunkUnits -
                       static_cast<float>(bounds.minY)) *
                          cell};
    }
};

Layout layoutFor(const Bounds& bounds) {
    const float columns = static_cast<float>(bounds.maxX - bounds.minX + 1);
    const float rows = static_cast<float>(bounds.maxY - bounds.minY + 1);
    const float width = static_cast<float>(GetScreenWidth()) - 80.0f;
    const float height = static_cast<float>(GetScreenHeight()) - 140.0f;
    const float cell = std::max(2.0f, std::min(width / columns, height / rows));
    return {bounds, cell, (static_cast<float>(GetScreenWidth()) - cell * columns) / 2.0f,
            90.0f + (height - cell * rows) / 2.0f};
}

void drawChunks(const MapState& map, const Layout& layout, const world::WorldTerrain* terrain) {
    for (const net::MapChunk& chunk : map.shared()) {
        DrawRectangleRec(layout.cellOf(chunk), chunkColor(chunk, terrain));
    }
    for (const net::MapChunk& chunk : map.pending()) {
        if (!map.shared().contains(chunk)) {
            const Rectangle cell = layout.cellOf(chunk);
            DrawRectangleRec(cell, kPending);
            DrawLineV({cell.x, cell.y + cell.height}, {cell.x + cell.width, cell.y}, kTable);
        }
    }
}

void drawMarkers(const MapMarkers& markers, const Layout& layout) {
    for (const net::EntityState& entity : markers.entities) {
        const Vector2 at = layout.pointOf(entity.position);
        if (entity.kind == net::EntityKind::Player) {
            const bool self = markers.self && entity.id == *markers.self;
            DrawCircleV(at, self ? 7.0f : 5.0f, self ? RED : BLUE);
        } else if (entity.kind == net::EntityKind::Station) {
            DrawRectangleV({at.x - 5, at.y - 5}, {10, 10}, kTable);
        }
    }
}

} // namespace

void drawMapScreen(const MapState& map, const MapMarkers& markers,
                   const world::WorldTerrain* terrain) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), kPaper);
    DrawText("Carte  (M : fermer)", 30, 24, 26, kInk);
    DrawText("Couleur : carte commune.  Jaune hachuré : souvenirs à reporter à une table.  Blanc : "
             "inconnu.",
             30, 58, 16, kInk);
    const Layout layout = layoutFor(boundsOf(map));
    drawChunks(map, layout, terrain);
    drawMarkers(markers, layout);
}

void drawMapReminder(const MapState& map) {
    const std::size_t count = map.toReport();
    if (count == 0) {
        return;
    }
    const std::string text =
        "Souvenirs à reporter à une table de cartographie : " + std::to_string(count) +
        " région(s)";
    DrawText(text.c_str(), GetScreenWidth() - MeasureText(text.c_str(), 18) - 20, 20, 18,
             Color{250, 225, 140, 255});
}

} // namespace odyssee::debugclient
