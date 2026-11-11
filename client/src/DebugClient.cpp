#include "DebugClient.hpp"

#include "Scene.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/ProvisionalGround.hpp"

#include <raylib.h>

#include <utility>

namespace odyssee::debugclient {

namespace {

constexpr log::Logger kLog{"client"};
constexpr float kTickSeconds = 1.0f / static_cast<float>(kTickRate);
constexpr int kMaxTicksPerFrame = 5; ///< après un gel, on ne rattrape pas indéfiniment

const char* describe(net::ConnectionState state) {
    switch (state) {
    case net::ConnectionState::Idle:
    case net::ConnectionState::Requesting:
    case net::ConnectionState::Answering: return "connexion...";
    case net::ConnectionState::Connected: return "connecté";
    case net::ConnectionState::Refused: return "connexion refusée";
    case net::ConnectionState::TimedOut: return "le serveur ne répond pas";
    case net::ConnectionState::Closed: return "déconnecté";
    }
    return "?";
}

} // namespace

DebugClient::DebugClient(client::ServerLink link)
    : _link(std::move(link)), _ground(sim::provisionalGroundHeight()), _prediction(_ground) {}

void DebugClient::run() {
    _link.connect(Clock::now());
    float accumulated = 0.0f;

    while (!WindowShouldClose()) {
        handleWindowKeys();
        _input.update();

        accumulated += GetFrameTime();
        for (int i = 0; i < kMaxTicksPerFrame && accumulated >= kTickSeconds; ++i) {
            tick(Clock::now());
            accumulated -= kTickSeconds;
        }
        if (accumulated >= kTickSeconds) {
            accumulated = 0.0f; // trop de retard : on repart du présent
        }

        draw(accumulated / kTickSeconds);
    }
    leave();
}

void DebugClient::handleWindowKeys() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        EnableCursor();
    }
    if (!IsCursorHidden() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        DisableCursor();
    }
    if (IsKeyPressed(KEY_V)) {
        _thirdPerson = !_thirdPerson;
    }
}

void DebugClient::tick(Clock::time_point now) {
    _link.poll(now);
    keepLatestSnapshot();
    startPredictionWhenPossible();

    const std::optional<sim::PlayerState> before = _prediction.state();
    if (_prediction.advance(_input.sample())) {
        _previous = before;
        _link.sendInputs(_prediction.history().recentBatch(), now);
    }
}

void DebugClient::keepLatestSnapshot() {
    for (auto& snapshot : _link.takeSnapshots()) {
        if (!_latest || snapshot.serverTick > _latest->serverTick) {
            _latest = std::move(snapshot); // un snapshot arrivé en retard est plus vieux : ignoré
        }
    }
}

void DebugClient::startPredictionWhenPossible() {
    const auto self = _link.playerId();
    if (_prediction.started() || !_latest || !self) {
        return;
    }
    if (_prediction.start(*_latest, *self)) {
        _input.lookTowards(net::dequantizeYaw(_prediction.state()->yaw));
        const Vector3 at = toWorld(_prediction.state()->position);
        kLog.info("joueur {} : prédiction démarrée en ({:.1f}, {:.1f}, {:.1f}) m", *self, at.x,
                  at.z, at.y);
    }
}

void DebugClient::draw(float betweenTicks) const {
    BeginDrawing();
    ClearBackground(Color{150, 195, 230, 255});
    if (_prediction.started()) {
        drawWorld(betweenTicks);
    }
    drawHud(hudInfo());
    EndDrawing();
}

void DebugClient::drawWorld(float betweenTicks) const {
    const Vector3 now = toWorld(_prediction.state()->position);
    const Vector3 feet = _previous ? lerp(toWorld(_previous->position), now, betweenTicks) : now;

    BeginMode3D(cameraFor(feet, _input.yawDegrees(), _input.pitchDegrees(), _thirdPerson));
    drawGround(feet, toWorld({0, 0, _ground.groundHeight(0, 0)}).y);
    for (const auto& entity : _latest->entities) {
        if (entity.id != _link.playerId()) {
            drawPlayer(toWorld(entity.position), net::dequantizeYaw(entity.yaw),
                       playerColor(entity.id));
        }
    }
    if (_thirdPerson) {
        drawPlayer(feet, _input.yawDegrees(), playerColor(_link.playerId().value_or(0)));
    }
    EndMode3D();
}

HudInfo DebugClient::hudInfo() const {
    HudInfo info;
    info.connection = describe(_link.state());
    if (const auto id = _link.playerId()) {
        info.playerId = *id;
    }
    if (_link.session() && _link.session()->roundTripTime()) {
        const auto rtt = *_link.session()->roundTripTime();
        info.pingMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(rtt).count());
    }
    if (const auto& state = _prediction.state()) {
        const Vector3 p = toWorld(state->position);
        info.x = p.x;
        info.y = p.z; // sud
        info.z = p.y; // hauteur
        info.stamina = static_cast<float>(state->stamina) /
                       static_cast<float>(sim::kDefaultTuning.maxStamina);
        info.exhausted = state->has(sim::kExhausted);
    }
    info.playersInView = _latest ? static_cast<int>(_latest->entities.size()) - 1 : 0;
    info.mouseCaptured = IsCursorHidden();
    return info;
}

void DebugClient::leave() {
    if (const auto& state = _prediction.state()) {
        const Vector3 at = toWorld(state->position);
        kLog.info("départ : {} inputs joués, dernière position ({:.1f}, {:.1f}, {:.1f}) m",
                  _prediction.history().lastTick(), at.x, at.z, at.y);
    }
    _link.disconnect();
    _link.poll(Clock::now()); // prévient le serveur : la place se libère tout de suite
}

} // namespace odyssee::debugclient
