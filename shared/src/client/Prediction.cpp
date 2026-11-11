#include "odyssee/client/Prediction.hpp"

#include "odyssee/sim/Replication.hpp"

#include <algorithm>
#include <cmath>

namespace odyssee::client {

namespace {

const net::EntityState* findEntity(const net::Snapshot& snapshot, net::EntityId id) {
    const auto it = std::find_if(snapshot.entities.begin(), snapshot.entities.end(),
                                 [id](const net::EntityState& e) { return e.id == id; });
    return it == snapshot.entities.end() ? nullptr : &*it;
}

/// Longueur d'un écart (1/256 m), arrondie.
std::int32_t lengthOf(const std::array<std::int32_t, 3>& v) {
    const double x = v[0];
    const double y = v[1];
    const double z = v[2];
    return static_cast<std::int32_t>(std::lround(std::sqrt(x * x + y * y + z * z)));
}

} // namespace

bool Prediction::start(const net::Snapshot& snapshot, net::EntityId self) {
    if (started()) {
        return true;
    }

    const net::EntityState* entity = findEntity(snapshot, self);
    if (!entity) {
        return false;
    }

    _state = sim::restore(*entity, snapshot.self);
    return true;
}

bool Prediction::advance(const net::InputCommand& command) {
    if (!started()) {
        return false;
    }

    _state = sim::step(*_state, command, _terrain);
    _history.record(_nextTick, command);
    ++_nextTick;
    fadeDisplayError();
    return true;
}

bool Prediction::reconcile(const net::Snapshot& snapshot, net::EntityId self) {
    const net::Tick processed = snapshot.lastProcessedInput;
    if (!started() || processed == 0 || processed <= _lastReconciled) {
        return false;
    }

    const net::EntityState* entity = findEntity(snapshot, self);
    if (!entity) {
        return false;
    }

    _lastReconciled = processed;
    applyCorrection(replayFrom(sim::restore(*entity, snapshot.self), processed));
    return true;
}

std::array<std::int32_t, 3> Prediction::displayPosition() const {
    if (!started()) {
        return {};
    }
    const auto& p = _state->position;
    return {p[0] + _displayError[0], p[1] + _displayError[1], p[2] + _displayError[2]};
}

sim::PlayerState Prediction::replayFrom(sim::PlayerState state, net::Tick tick) const {
    // Inputs plus anciens que l'historique (très long gel) : on ne peut que reprendre l'état du
    // serveur tel quel, la prédiction repart de là.
    for (const net::InputCommand& command : _history.after(tick)) {
        state = sim::step(state, command, _terrain);
    }
    return state;
}

void Prediction::applyCorrection(const sim::PlayerState& corrected) {
    std::array<std::int32_t, 3> error{};
    for (std::size_t i = 0; i < 3; ++i) {
        error[i] = _state->position[i] - corrected.position[i];
    }

    const std::int32_t distance = lengthOf(error);
    if (distance > 0) {
        ++_corrections;
        _lastCorrection = distance;
    }

    // Petit écart : l'affichage reste où il était et rejoint la vraie position en douceur.
    // Grand écart (téléportation, gros décalage) : on recale tout de suite.
    for (std::size_t i = 0; i < 3; ++i) {
        _displayError[i] = distance > kSnapDistance ? 0 : _displayError[i] + error[i];
    }
    _state = corrected;
}

void Prediction::fadeDisplayError() {
    for (std::int32_t& component : _displayError) {
        component = component * 3 / 4; // ~0,5 s pour effacer un écart d'un mètre
    }
}

} // namespace odyssee::client
