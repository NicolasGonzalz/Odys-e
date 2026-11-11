#include "GameStep.hpp"

namespace odyssee::server {

void stepGame(ConnectionService& connections, GameWorld& world, net::Tick tick,
              std::chrono::steady_clock::time_point now) {
    connections.poll(now);

    const auto players = connections.connectedPlayers();
    world.syncPlayers(players);
    for (const net::PlayerId player : players) {
        for (const auto& batch : connections.takeInputs(player)) {
            world.receiveInputs(player, batch);
        }
    }

    world.step();

    for (const net::PlayerId player : players) {
        connections.queueSnapshot(player, world.snapshotFor(player, tick));
    }
    connections.flushGame(now);
}

} // namespace odyssee::server
