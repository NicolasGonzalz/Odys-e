#include "MapView.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/world/CollisionShape.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace odyssee::debugclient {

namespace {

constexpr std::int64_t kChunkRadius = 2;   ///< blocs gardés autour du joueur : 5 × 5 (320 m)
constexpr int kChunksBuiltPerFrame = 2;    ///< pas de gel quand on entre dans une zone neuve
constexpr float kBoxDrawDistance = 150.0f; ///< volumes dessinés jusqu'à cette distance
constexpr float kYawToDegrees = 360.0f / 65536.0f;

/// Un sommet du maillage du relief, déjà en repère raylib (X = x, Y = z, Z = y).
struct Vertex {
    Vector3 position;
    Color color;
};

/// Couleur d'un point selon sa hauteur et la pente (éclairage simple, venu du nord-ouest).
Color shade(float height, Vector3 normal) {
    const float light = std::clamp(
        0.55f + 0.45f * (normal.y * 0.8f - normal.x * 0.3f - normal.z * 0.3f), 0.3f, 1.0f);
    const float t = std::clamp((height - 0.0f) / 120.0f, 0.0f, 1.0f); // plaine verte → roche
    const auto channel = [&](float low, float high) {
        return static_cast<unsigned char>(
            std::clamp((low + (high - low) * t) * light, 0.0f, 255.0f));
    };
    return Color{channel(92, 140), channel(140, 128), channel(70, 110), 255};
}

Vector3 normalOf(Vector3 a, Vector3 b, Vector3 c) {
    const Vector3 u{b.x - a.x, b.y - a.y, b.z - a.z};
    const Vector3 v{c.x - a.x, c.y - a.y, c.z - a.z};
    Vector3 n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
    const float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    return length > 0 ? Vector3{n.x / length, n.y / length, n.z / length} : Vector3{0, 1, 0};
}

/// Point (col, row) du relief, en repère raylib.
Vector3 pointAt(const world::HeightField& relief, std::int64_t col, std::int64_t row) {
    const float size = net::dequantizePosition(static_cast<std::int32_t>(relief.quadSize));
    return {static_cast<float>(col) * size, net::dequantizePosition(relief.height(col, row)),
            static_cast<float>(row) * size};
}

/// Ajoute un triangle, sommets donnés face au ciel dans le sens inverse des aiguilles.
void addTriangle(std::vector<Vertex>& out, Vector3 a, Vector3 b, Vector3 c) {
    const Vector3 normal = normalOf(a, b, c);
    for (const Vector3 p : {a, b, c}) {
        out.push_back({p, shade(p.y, normal)});
    }
}

/// Les deux triangles d'une case, coupée comme dans la simulation : de (col, row) à
/// (col + 1, row + 1).
void addQuad(std::vector<Vertex>& out, const world::HeightField& relief, std::int64_t col,
             std::int64_t row) {
    const Vector3 p00 = pointAt(relief, col, row);
    const Vector3 p10 = pointAt(relief, col + 1, row);
    const Vector3 p01 = pointAt(relief, col, row + 1);
    const Vector3 p11 = pointAt(relief, col + 1, row + 1);
    addTriangle(out, p00, p11, p10);
    addTriangle(out, p00, p01, p11);
}

/// Maillage raylib (mémoire allouée par raylib : UnloadModel la libère).
Mesh toMesh(const std::vector<Vertex>& vertices) {
    Mesh mesh{};
    mesh.vertexCount = static_cast<int>(vertices.size());
    mesh.triangleCount = mesh.vertexCount / 3;
    const auto count = static_cast<unsigned int>(vertices.size());
    mesh.vertices = static_cast<float*>(MemAlloc(count * 3 * sizeof(float)));
    mesh.colors = static_cast<unsigned char*>(MemAlloc(count * 4));
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const Vertex& v = vertices[i];
        mesh.vertices[3 * i] = v.position.x;
        mesh.vertices[3 * i + 1] = v.position.y;
        mesh.vertices[3 * i + 2] = v.position.z;
        mesh.colors[4 * i] = v.color.r;
        mesh.colors[4 * i + 1] = v.color.g;
        mesh.colors[4 * i + 2] = v.color.b;
        mesh.colors[4 * i + 3] = v.color.a;
    }
    UploadMesh(&mesh, false);
    return mesh;
}

/// Teinte d'un volume : sols et plafonds (plats) sombres, murs clairs, cylindres (capsules,
/// piliers) bleutés. Rien de tout cela n'est dans le fichier : juste pour lire la scène.
Color tintOf(const world::CollisionBox& box) {
    if (box.shape == world::BoxShape::Cylinder) {
        return Color{120, 150, 170, 255};
    }
    const bool flat = box.halfExtents[2] < std::min(box.halfExtents[0], box.halfExtents[1]);
    return flat ? Color{105, 100, 95, 255} : Color{175, 165, 150, 255};
}

void drawBox(const world::CollisionBox& box) {
    const auto meters = [](std::int64_t units) {
        return net::dequantizePosition(static_cast<std::int32_t>(units));
    };
    const float hx = meters(box.halfExtents[0]);
    const float hy = meters(box.halfExtents[1]);
    const float hz = meters(box.halfExtents[2]);

    rlPushMatrix();
    rlTranslatef(meters(box.center[0]), meters(box.center[2]), meters(box.center[1]));
    // Le cap tourne de +x (est) vers +y (sud) ; autour de l'axe vertical de raylib, c'est
    // l'angle opposé.
    rlRotatef(-static_cast<float>(box.yaw) * kYawToDegrees, 0.0f, 1.0f, 0.0f);
    if (box.shape == world::BoxShape::Cylinder) {
        rlScalef(1.0f, 1.0f, hy / hx);
        DrawCylinder({0, -hz, 0}, hx, hx, 2 * hz, 16, tintOf(box));
        DrawCylinderWires({0, -hz, 0}, hx, hx, 2 * hz, 16, Color{60, 55, 50, 255});
    } else {
        DrawCube({0, 0, 0}, 2 * hx, 2 * hz, 2 * hy, tintOf(box));
        DrawCubeWires({0, 0, 0}, 2 * hx, 2 * hz, 2 * hy, Color{60, 55, 50, 255});
    }
    rlPopMatrix();
}

} // namespace

MapView::MapView(const world::WorldFile& world) : _world(world) {}

MapView::~MapView() {
    for (auto& [key, model] : _chunks) {
        UnloadModel(model);
    }
}

void MapView::update(Vector3 around) {
    const float chunkMeters =
        net::dequantizePosition(static_cast<std::int32_t>(_world.terrain.quadSize)) *
        static_cast<float>(_world.terrain.chunkQuads);
    const ChunkKey center{static_cast<std::int64_t>(std::floor(around.x / chunkMeters)),
                          static_cast<std::int64_t>(std::floor(around.z / chunkMeters))};
    forgetFarChunks(center);

    int built = 0;
    for (std::int64_t dy = -kChunkRadius; dy <= kChunkRadius && built < kChunksBuiltPerFrame;
         ++dy) {
        for (std::int64_t dx = -kChunkRadius; dx <= kChunkRadius && built < kChunksBuiltPerFrame;
             ++dx) {
            const ChunkKey key{center.first + dx, center.second + dy};
            const bool onMap = key.first >= 0 && key.second >= 0 &&
                               key.first < _world.terrain.chunksX &&
                               key.second < _world.terrain.chunksY;
            if (onMap && !_chunks.contains(key)) {
                _chunks.emplace(key, buildChunk(key));
                ++built;
            }
        }
    }
}

void MapView::draw(Vector3 around) const {
    for (const auto& [key, model] : _chunks) {
        DrawModel(model, {0, 0, 0}, 1.0f, WHITE);
    }
    for (const world::CollisionBox& box : _world.boxes) {
        const float dx = net::dequantizePosition(box.center[0]) - around.x;
        const float dz = net::dequantizePosition(box.center[1]) - around.z;
        if (dx * dx + dz * dz < kBoxDrawDistance * kBoxDrawDistance) {
            drawBox(box);
        }
    }
}

void MapView::forgetFarChunks(ChunkKey center) {
    for (auto it = _chunks.begin(); it != _chunks.end();) {
        if (isNear(it->first, center)) {
            ++it;
            continue;
        }
        UnloadModel(it->second);
        it = _chunks.erase(it);
    }
}

Model MapView::buildChunk(ChunkKey key) const {
    const world::HeightField& relief = _world.terrain;
    const std::int64_t firstCol = key.first * relief.chunkQuads;
    const std::int64_t firstRow = key.second * relief.chunkQuads;
    const std::int64_t lastCol =
        std::min<std::int64_t>(firstCol + relief.chunkQuads, relief.quadsX);
    const std::int64_t lastRow =
        std::min<std::int64_t>(firstRow + relief.chunkQuads, relief.quadsY);

    std::vector<Vertex> vertices;
    for (std::int64_t row = firstRow; row < lastRow; ++row) {
        for (std::int64_t col = firstCol; col < lastCol; ++col) {
            const world::QuadCoord quad{static_cast<std::uint32_t>(col),
                                        static_cast<std::uint32_t>(row)};
            if (!_world.isHole(quad)) { // entrée de grotte ou du bunker : on voit dessous
                addQuad(vertices, relief, col, row);
            }
        }
    }
    return LoadModelFromMesh(toMesh(vertices));
}

bool MapView::isNear(ChunkKey key, ChunkKey center) const {
    return std::llabs(key.first - center.first) <= kChunkRadius &&
           std::llabs(key.second - center.second) <= kChunkRadius;
}

} // namespace odyssee::debugclient
