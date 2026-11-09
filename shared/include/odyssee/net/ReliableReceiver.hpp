#pragma once

#include "odyssee/net/ReliableMessage.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace odyssee::net {

/// Côté réception du canal fiable : ignore les doublons, met de côté les messages arrivés trop tôt,
/// et livre les contenus au jeu **dans l'ordre** des numéros.
class ReliableReceiver {
  public:
    /// Prend en compte un message reçu (nouveau, en avance, ou doublon).
    void receive(ReliableMessage message);

    /// Retire les contenus livrables dans l'ordre (peut être vide si un message manque encore).
    [[nodiscard]] std::vector<std::vector<std::byte>> takeDeliverable();

    /// Messages reçus en avance, en attente de ceux qui les précèdent.
    [[nodiscard]] std::size_t waitingCount() const { return _early.size(); }

  private:
    [[nodiscard]] bool isAlreadyDelivered(MessageId id) const;

    MessageId _nextExpected = 0;
    std::unordered_map<MessageId, std::vector<std::byte>> _early;
};

} // namespace odyssee::net
