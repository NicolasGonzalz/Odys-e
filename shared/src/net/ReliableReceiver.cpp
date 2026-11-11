#include "odyssee/net/ReliableReceiver.hpp"

#include "odyssee/net/Sequence.hpp"

namespace odyssee::net {

void ReliableReceiver::receive(ReliableMessage message) {
    if (isAlreadyDelivered(message.id)) {
        return; // renvoi d'un message déjà livré (son acquittement s'était perdu)
    }
    if (sequenceDistance(message.id, _nextExpected) >= kMaxAhead) {
        return;
    }
    // try_emplace n'écrase pas : un doublon d'un message en attente est ignoré aussi.
    _early.try_emplace(message.id, std::move(message.payload));
}

std::vector<std::vector<std::byte>> ReliableReceiver::takeDeliverable() {
    std::vector<std::vector<std::byte>> delivered;

    for (auto next = _early.find(_nextExpected); next != _early.end();
         next = _early.find(_nextExpected)) {
        delivered.push_back(std::move(next->second));
        _early.erase(next);
        ++_nextExpected;
    }
    return delivered;
}

bool ReliableReceiver::isAlreadyDelivered(MessageId id) const {
    // Déjà livré = strictement avant le prochain attendu (comparaison avec rebouclage).
    return isMoreRecent(_nextExpected, id);
}

} // namespace odyssee::net
