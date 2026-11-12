#include "InputSampler.hpp"

#include "odyssee/net/Quantize.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>

namespace odyssee::debugclient {

namespace {

constexpr float kMouseDegreesPerPixel = 0.12f;
constexpr float kMaxPitch = 89.0f;

float axis(int positiveKey, int negativeKey) {
    return (IsKeyDown(positiveKey) ? 1.0f : 0.0f) - (IsKeyDown(negativeKey) ? 1.0f : 0.0f);
}

std::uint16_t pressedButtons() {
    std::uint16_t buttons = 0;
    const auto add = [&buttons](bool down, net::InputButton button) {
        if (down) {
            buttons = static_cast<std::uint16_t>(buttons | static_cast<std::uint16_t>(button));
        }
    };
    add(IsKeyDown(KEY_SPACE), net::InputButton::Jump);
    add(IsKeyDown(KEY_LEFT_SHIFT), net::InputButton::Sprint);
    add(IsKeyDown(KEY_LEFT_CONTROL), net::InputButton::Crouch);
    add(IsKeyDown(KEY_E), net::InputButton::Use);
    add(IsMouseButtonDown(MOUSE_BUTTON_LEFT), net::InputButton::Primary);
    add(IsMouseButtonDown(MOUSE_BUTTON_RIGHT), net::InputButton::Secondary);
    return buttons;
}

} // namespace

void InputSampler::update() {
    if (!IsCursorHidden()) {
        return; // souris libre (Échap) : la caméra ne bouge pas
    }
    const Vector2 delta = GetMouseDelta();
    _yaw = std::fmod(_yaw + delta.x * kMouseDegreesPerPixel + 360.0f, 360.0f);
    _pitch = std::clamp(_pitch - delta.y * kMouseDegreesPerPixel, -kMaxPitch, kMaxPitch);
}

net::InputCommand InputSampler::sample() const {
    net::InputCommand command;
    if (!IsCursorHidden()) {
        // Souris libre (menu Échap, fenêtre quittée) : le joueur ne bouge plus, il garde son cap.
        command.yaw = net::quantizeYaw(_yaw);
        command.pitch = net::quantizePitch(_pitch);
        return command;
    }
    command.moveForward = net::quantizeAxis(axis(KEY_W, KEY_S));
    command.moveRight = net::quantizeAxis(axis(KEY_D, KEY_A));
    command.yaw = net::quantizeYaw(_yaw);
    command.pitch = net::quantizePitch(_pitch);
    command.buttons = pressedButtons();
    return command;
}

} // namespace odyssee::debugclient
