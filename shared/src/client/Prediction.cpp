#include "odyssee/client/Prediction.hpp"

#include "odyssee/sim/Replication.hpp"

#include <algorithm>

namespace odyssee::client {

bool Prediction::start(const net::Snapshot& snapshot, net::EntityId self) {
    if (started()) {
        return true;
    }

    const auto it = std::find_if(snapshot.entities.begin(), snapshot.entities.end(),
                                 [self](const net::EntityState& e) { return e.id == self; });
    if (it == snapshot.entities.end()) {
        return false;
    }

    _state = sim::restore(*it, snapshot.self);
    return true;
}

bool Prediction::advance(const net::InputCommand& command) {
    if (!started()) {
        return false;
    }

    _state = sim::step(*_state, command, _terrain);
    _history.record(_nextTick, command);
    ++_nextTick;
    return true;
}

} // namespace odyssee::client
