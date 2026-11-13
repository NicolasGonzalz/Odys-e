#include "DebugClient.hpp"

#include "PauseMenu.hpp"
#include "ResourceShapes.hpp"
#include "Scene.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/ProvisionalGround.hpp"
#include "odyssee/world/WorldFingerprint.hpp"

#include <raylib.h>

#include <algorithm>
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

/// Les autres joueurs du snapshot (sans soi, ni les ressources).
int otherPlayersIn(const net::Snapshot& snapshot) {
    const auto players =
        std::count_if(snapshot.entities.begin(), snapshot.entities.end(),
                      [](const net::EntityState& e) { return e.kind == net::EntityKind::Player; });
    return std::max(0, static_cast<int>(players) - 1);
}

} // namespace

DebugClient::DebugClient(client::ServerLink link, GameData data)
    : _link(std::move(link)), _map(data.map), _resourceTypes(data.resourceTypes),
      _catalog(data.catalog),
      _flat(sim::provisionalGroundHeight()),
      _prediction(_map != nullptr ? static_cast<const sim::Terrain&>(*_map) : _flat) {
    if (_map != nullptr) {
        _mapView.emplace(_map->world());
    }
}

SessionEnd DebugClient::run() {
    _link.connect(Clock::now());
    DisableCursor();
    float accumulated = 0.0f;

    while (!WindowShouldClose()) {
        handleWindowKeys();
        _input.update();
        advance(accumulated);

        const PauseChoice choice = draw(accumulated / kTickSeconds);
        if (const auto end = endAfter(choice)) {
            leave();
            return *end;
        }
    }
    leave();
    return {true, {}};
}

void DebugClient::advance(float& accumulated) {
    accumulated += GetFrameTime();
    for (int i = 0; i < kMaxTicksPerFrame && accumulated >= kTickSeconds; ++i) {
        tick(Clock::now());
        accumulated -= kTickSeconds;
    }
    if (accumulated >= kTickSeconds) {
        accumulated = 0.0f; // trop de retard : on repart du présent
    }

    _remote.advance(GetFrameTime() * static_cast<float>(kTickRate));
    if (_mapView && _prediction.started()) {
        _mapView->update(toWorld(_prediction.displayPosition()));
    }
}

std::optional<SessionEnd> DebugClient::endAfter(PauseChoice choice) {
    if (choice == PauseChoice::Resume) {
        setPaused(false);
    }
    if (choice == PauseChoice::Disconnect) {
        return SessionEnd{false, {}};
    }
    if (choice == PauseChoice::Quit) {
        return SessionEnd{true, {}};
    }
    return connectionLost();
}

std::optional<SessionEnd> DebugClient::connectionLost() const {
    if (const auto serverMap = _link.serverWorldFingerprint(); serverMap && *serverMap != ownMap()) {
        return SessionEnd{false, *serverMap == world::kNoMapFingerprint
                                     ? "Le serveur tourne sans carte : relancez sans --world"
                                     : "Carte différente de celle du serveur (mettez à jour le jeu)"};
    }
    switch (_link.state()) {
    case net::ConnectionState::Refused: {
        const auto reason = _link.refuseReason();
        const std::string why = reason ? std::string(net::describe(*reason)) : "inconnue";
        return SessionEnd{false, "Connexion refusée : " + why};
    }
    case net::ConnectionState::TimedOut:
        return SessionEnd{false, "Le serveur ne répond pas (adresse, port ou pare-feu ?)"};
    case net::ConnectionState::Closed:
        return SessionEnd{false, "Déconnecté par le serveur"};
    case net::ConnectionState::Idle:
    case net::ConnectionState::Requesting:
    case net::ConnectionState::Answering:
    case net::ConnectionState::Connected:
        return std::nullopt;
    }
    return std::nullopt;
}

std::uint64_t DebugClient::ownMap() const {
    return _map != nullptr ? _map->world().fingerprint : world::kNoMapFingerprint;
}

void DebugClient::handleWindowKeys() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        setPaused(!_paused);
    }
    if (!_paused && !IsCursorHidden() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        DisableCursor(); // la souris avait été libérée (fenêtre quittée) : on la reprend
    }
    if (IsKeyPressed(KEY_V)) {
        _thirdPerson = !_thirdPerson;
    }
}

void DebugClient::setPaused(bool paused) {
    _paused = paused;
    if (paused) {
        EnableCursor();
    } else {
        DisableCursor();
    }
}

void DebugClient::tick(Clock::time_point now) {
    _link.poll(now);
    keepLatestSnapshot();
    keepEvents();
    startPredictionWhenPossible();

    const auto before = _prediction.displayPosition();
    if (_prediction.advance(_input.sample())) {
        _previousDisplay = before;
        _link.sendInputs(_prediction.history().recentBatch(), now);
    }
}

void DebugClient::keepLatestSnapshot() {
    for (auto& snapshot : _link.takeSnapshots()) {
        _remote.push(snapshot); // T033 : tous les snapshots, même en retard, servent à interpoler
        if (!_latest || snapshot.serverTick > _latest->serverTick) {
            _latest = std::move(snapshot); // un snapshot arrivé en retard est plus vieux : ignoré
            reconcileWithLatest();
        }
    }
}

void DebugClient::keepEvents() {
    for (auto& event : _link.takeEvents()) {
        if (auto* contents = std::get_if<net::InventoryContents>(&event)) {
            _inventory = std::move(*contents); // le serveur renvoie tout à chaque changement
        }
    }
}

std::string DebugClient::hotbarText() const {
    if (!_inventory) {
        return "Inventaire : en attente du serveur";
    }
    std::string text = "Outils :";
    for (std::size_t i = 0; i < net::kHotbarSlots; ++i) {
        const net::InventorySlot& slot = _inventory->slots[i];
        const data::ItemDef* def = _catalog ? _catalog->item(data::ItemId{slot.item}) : nullptr;
        const std::string name =
            slot.count == 0 ? "-" : (def ? def->name : "objet " + std::to_string(slot.item));
        text += "  " + std::to_string(i + 1) + ":" + name;
        text += slot.count > 1 ? " x" + std::to_string(slot.count) : "";
    }
    return text;
}

void DebugClient::reconcileWithLatest() {
    if (const auto self = _link.playerId()) {
        _prediction.reconcile(*_latest, *self); // T032 : recalage sur la vérité du serveur
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

PauseChoice DebugClient::draw(float betweenTicks) const {
    BeginDrawing();
    ClearBackground(Color{150, 195, 230, 255});
    if (_prediction.started()) {
        drawWorld(betweenTicks);
    }
    drawHud(hudInfo());
    const PauseChoice choice = _paused ? drawPauseMenu() : PauseChoice::None;
    EndDrawing();
    return choice;
}

void DebugClient::drawWorld(float betweenTicks) const {
    const Vector3 now = toWorld(_prediction.displayPosition());
    const Vector3 feet = _previousDisplay ? lerp(toWorld(*_previousDisplay), now, betweenTicks) : now;

    BeginMode3D(cameraFor(feet, _input.yawDegrees(), _input.pitchDegrees(), _thirdPerson));
    if (_mapView) {
        _mapView->draw(feet);
    } else {
        drawGround(feet, net::dequantizePosition(_flat.groundHeight(0, 0)));
    }
    if (_latest) {
        drawResources(_latest->entities, _resourceTypes); // T044 : fixes, dernier état connu
    }
    for (const auto& entity : _remote.sample(_link.playerId())) { // T033 : interpolés
        drawPlayer(toWorld(entity.position), static_cast<float>(entity.yawDegrees),
                   playerColor(entity.id));
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
    const auto traffic = _link.traffic(Clock::now());
    info.lossPercent = static_cast<float>(traffic.incomingLossPercent);
    info.uploadKbps = static_cast<float>(traffic.sentKilobytesPerSecond);
    info.downloadKbps = static_cast<float>(traffic.receivedKilobytesPerSecond);
    info.playersInView = _latest ? otherPlayersIn(*_latest) : 0;
    info.hotbar = hotbarText();
    if (_latest) {
        info.health = _latest->vitals.health;
        info.hunger = _latest->vitals.hunger;
        info.thirst = _latest->vitals.thirst;
    }
    info.corrections = static_cast<int>(_prediction.corrections());
    info.lastCorrectionCm = net::dequantizePosition(_prediction.lastCorrection()) * 100.0f;
    info.mouseCaptured = IsCursorHidden();
    return info;
}

void DebugClient::leave() {
    if (const auto& state = _prediction.state()) {
        const Vector3 at = toWorld(state->position);
        kLog.info("départ : {} inputs joués, {} corrections, dernière position ({:.1f}, {:.1f}, "
                  "{:.1f}) m",
                  _prediction.history().lastTick(), _prediction.corrections(), at.x, at.z, at.y);
    }
    _link.disconnect();
    _link.poll(Clock::now()); // prévient le serveur : la place se libère tout de suite
}

} // namespace odyssee::debugclient
