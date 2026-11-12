#include "odyssee/client/Interpolation.hpp"

#include "odyssee/net/Quantize.hpp"

#include <algorithm>
#include <cmath>

namespace odyssee::client {

namespace {

/// Recalage de l'horloge d'affichage : au plus ±25 % de vitesse, proportionnel à l'écart.
constexpr double kCatchUpGain = 0.1;
constexpr double kMaxCatchUp = 0.25;

const net::EntityState* find(const std::vector<net::EntityState>& entities, net::EntityId id) {
    const auto it = std::find_if(entities.begin(), entities.end(),
                                 [id](const net::EntityState& e) { return e.id == id; });
    return it == entities.end() ? nullptr : &*it;
}

/// Écart d'angle le plus court de `from` vers `to`, en degrés (-180 à 180).
double shortestTurn(double from, double to) {
    double delta = std::fmod(to - from + 540.0, 360.0) - 180.0;
    return delta == -180.0 ? 180.0 : delta;
}

} // namespace

void Interpolation::push(const net::Snapshot& snapshot) {
    const auto at = std::lower_bound(
        _frames.begin(), _frames.end(), snapshot.serverTick,
        [](const Frame& frame, net::Tick tick) { return frame.tick < tick; });
    if (at != _frames.end() && at->tick == snapshot.serverTick) {
        return; // déjà connu (doublon)
    }
    if (_renderTick && static_cast<double>(snapshot.serverTick) < *_renderTick - 1.0) {
        return; // arrivé trop tard : l'affichage est déjà passé
    }

    _frames.insert(at, Frame{snapshot.serverTick, snapshot.entities});
    if (!_renderTick) {
        _renderTick = targetTick();
    }
    forgetOldFrames();
}

void Interpolation::advance(double ticks) {
    if (!_renderTick || _frames.empty()) {
        return;
    }

    const double gap = targetTick() - *_renderTick;
    if (std::abs(gap) > kSnapTicks) {
        _renderTick = targetTick();
        return;
    }

    // En retard : on accélère un peu ; en avance (snapshots qui manquent) : on ralentit.
    const double rate = 1.0 + std::clamp(gap * kCatchUpGain, -kMaxCatchUp, kMaxCatchUp);
    *_renderTick += ticks * rate;
}

std::vector<ShownEntity> Interpolation::sample(std::optional<net::EntityId> except) const {
    if (!_renderTick || _frames.empty()) {
        return {};
    }

    // Les deux images qui encadrent l'instant affiché (ou la plus proche, aux bords).
    const double now = *_renderTick;
    auto after = std::find_if(_frames.begin(), _frames.end(),
                              [now](const Frame& f) { return static_cast<double>(f.tick) > now; });
    const Frame& a = after == _frames.begin() ? *after : *std::prev(after);
    const Frame* b = after == _frames.end() || after == _frames.begin() ? nullptr : &*after;
    const double t = b ? (now - a.tick) / static_cast<double>(b->tick - a.tick) : 0.0;

    std::vector<ShownEntity> shown;
    for (const net::EntityState& entity : a.entities) {
        if (except && entity.id == *except) {
            continue;
        }
        shown.push_back(blend(entity, b ? find(b->entities, entity.id) : nullptr, t));
    }
    return shown;
}

double Interpolation::targetTick() const {
    return static_cast<double>(_frames.back().tick) - kDelayTicks;
}

void Interpolation::forgetOldFrames() {
    // On garde toujours l'image juste avant l'instant affiché (bord gauche de l'intervalle).
    while (_frames.size() > kCapacity ||
           (_frames.size() > 2 && _renderTick &&
            static_cast<double>(_frames[1].tick) <= *_renderTick)) {
        _frames.pop_front();
    }
}

ShownEntity Interpolation::blend(const net::EntityState& a, const net::EntityState* b, double t) {
    ShownEntity shown{a.id, {}, net::dequantizeYaw(a.yaw)};
    for (std::size_t i = 0; i < 3; ++i) {
        const double from = a.position[i];
        shown.position[i] = b ? from + (static_cast<double>(b->position[i]) - from) * t : from;
    }
    if (b) {
        shown.yawDegrees += shortestTurn(shown.yawDegrees, net::dequantizeYaw(b->yaw)) * t;
    }
    return shown;
}

} // namespace odyssee::client
