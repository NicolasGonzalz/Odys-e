#include "Scene.hpp"

#include "odyssee/net/Quantize.hpp"

#include <cmath>

namespace odyssee::debugclient {

namespace {

constexpr float kEyeHeight = 1.6f;
constexpr float kPlayerHeight = 1.8f;
constexpr float kPlayerWidth = 0.6f;
constexpr float kThirdPersonDistance = 4.0f;
constexpr float kGroundHalfSize = 60.0f;
constexpr float kGridStep = 2.0f;
constexpr float kDegToRad = 3.14159265f / 180.0f;

/// Direction du regard (raylib) : cap autour de la verticale, inclinaison vers le haut.
Vector3 lookDirection(float yawDegrees, float pitchDegrees) {
    const float yaw = yawDegrees * kDegToRad;
    const float pitch = pitchDegrees * kDegToRad;
    return {std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)};
}

Vector3 add(Vector3 a, Vector3 b, float scale) {
    return {a.x + b.x * scale, a.y + b.y * scale, a.z + b.z * scale};
}

} // namespace

Vector3 toWorld(const std::array<std::int32_t, 3>& position) {
    return {net::dequantizePosition(position[0]), net::dequantizePosition(position[2]),
            net::dequantizePosition(position[1])};
}

Vector3 toWorld(const std::array<double, 3>& position) {
    const auto meters = [](double units) {
        return static_cast<float>(units / static_cast<double>(net::kPositionUnitsPerMeter));
    };
    return {meters(position[0]), meters(position[2]), meters(position[1])};
}

Vector3 lerp(Vector3 from, Vector3 to, float t) {
    return {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t,
            from.z + (to.z - from.z) * t};
}

Camera3D cameraFor(Vector3 feet, float yawDegrees, float pitchDegrees, bool thirdPerson) {
    const Vector3 eye{feet.x, feet.y + kEyeHeight, feet.z};
    const Vector3 look = lookDirection(yawDegrees, pitchDegrees);

    Camera3D camera{};
    camera.position = thirdPerson ? add(add(eye, look, -kThirdPersonDistance), {0, 1, 0}, 0.8f)
                                  : eye;
    camera.target = thirdPerson ? eye : add(eye, look, 1.0f);
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

void drawGround(Vector3 around, float groundY) {
    const float cx = std::round(around.x / kGridStep) * kGridStep;
    const float cz = std::round(around.z / kGridStep) * kGridStep;
    DrawPlane({cx, groundY, cz}, {kGroundHalfSize * 2, kGroundHalfSize * 2}, Color{96, 150, 78, 255});

    const Color line{70, 120, 60, 255};
    for (float d = -kGroundHalfSize; d <= kGroundHalfSize; d += kGridStep) {
        DrawLine3D({cx + d, groundY + 0.01f, cz - kGroundHalfSize},
                   {cx + d, groundY + 0.01f, cz + kGroundHalfSize}, line);
        DrawLine3D({cx - kGroundHalfSize, groundY + 0.01f, cz + d},
                   {cx + kGroundHalfSize, groundY + 0.01f, cz + d}, line);
    }
}

void drawPlayer(Vector3 feet, float yawDegrees, Color color) {
    const Vector3 center{feet.x, feet.y + kPlayerHeight / 2, feet.z};
    DrawCube(center, kPlayerWidth, kPlayerHeight, kPlayerWidth, color);
    DrawCubeWires(center, kPlayerWidth, kPlayerHeight, kPlayerWidth, BLACK);

    const Vector3 eye{feet.x, feet.y + kEyeHeight, feet.z};
    DrawLine3D(eye, add(eye, lookDirection(yawDegrees, 0.0f), 1.0f), BLACK);
}

Color playerColor(std::uint16_t id) {
    static constexpr Color kColors[] = {{230, 140, 60, 255}, {70, 140, 230, 255},
                                        {200, 70, 160, 255}, {240, 210, 70, 255}};
    return kColors[id % 4];
}

} // namespace odyssee::debugclient
