#include "odyssee/sim/Replication.hpp"

namespace odyssee::sim {

net::EntityState toEntity(net::EntityId id, const PlayerState& state) {
    return net::EntityState{id, net::EntityKind::Player, state.position, state.yaw, state.flags};
}

net::OwnMotion toOwnMotion(const PlayerState& state) {
    return net::OwnMotion{state.velocity, state.stamina};
}

PlayerState restore(const net::EntityState& entity, const net::OwnMotion& own) {
    PlayerState state;
    state.position = entity.position;
    state.velocity = own.velocity;
    state.yaw = entity.yaw;
    state.pitch = 0;
    state.stamina = own.stamina;
    state.flags = entity.flags;
    return state;
}

} // namespace odyssee::sim
