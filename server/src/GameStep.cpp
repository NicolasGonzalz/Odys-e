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
        for (const auto& event : connections.takeEvents(player.id)) {
            world.receiveRequest(player.id, event);
        }
    }

    world.step();

    for (const ConnectedPlayer& player : players) {
        connections.queueSnapshot(player.id, world.snapshotFor(player.id, tick));
        for (const auto& event : world.takeEvents(player.id)) {
            connections.queueEvent(player.id, event); // réponses d'artisanat (T047)…
        }
        if (const auto inventory = world.takeInventoryUpdate(player.id)) {
            connections.queueEvent(player.id, *inventory); // T045 : seulement s'il a changé
        }
    }
    connections.flushGame(now);
}

} // namespace odyssee::server
