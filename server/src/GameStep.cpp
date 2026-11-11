#include "GameStep.hpp"

namespace odyssee::server {

void stepGame(ConnectionService& connections, GameWorld& world, net::Tick tick,
              std::chrono::steady_clock::time_point now) {
    connections.poll(now);

    const auto players = connections.connectedPlayers();
    world.syncPlayers(players);
    for (const ConnectedPlayer& player : players) {
        for (const auto& batch : connections.takeInputs(player.id)) {
            world.receiveInputs(player.id, batch);
        }
    }

    world.step();

    for (const ConnectedPlayer& player : players) {
        connections.queueSnapshot(player.id, world.snapshotFor(player.id, tick));
    }
    connections.flushGame(now);
}

} // namespace odyssee::server
