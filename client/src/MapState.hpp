#pragma once

#include "odyssee/net/GameEvents.hpp"

#include <cstddef>
#include <set>

namespace odyssee::debugclient {

/// Ce que le client sait de la carte (T074), d'après les événements MapChunks du serveur : la
/// carte commune du monde et ses propres souvenirs, pas encore reportés à une table.
class MapState {
  public:
    /// Traite un événement s'il concerne la carte ; faux sinon.
    bool onEvent(const net::GameEvent& event);

    [[nodiscard]] const std::set<net::MapChunk>& shared() const { return _shared; }
    [[nodiscard]] const std::set<net::MapChunk>& pending() const { return _pending; }
    /// Souvenirs qui ne sont pas encore sur la carte commune (un autre a pu les reporter avant).
    [[nodiscard]] std::size_t toReport() const;

    void toggle() { _open = !_open; }
    [[nodiscard]] bool open() const { return _open; }

  private:
    std::set<net::MapChunk> _shared;
    std::set<net::MapChunk> _pending;
    bool _open = false;
};

} // namespace odyssee::debugclient
