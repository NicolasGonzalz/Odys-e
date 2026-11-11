#include "InputQueue.hpp"

namespace odyssee::server {

void InputQueue::push(const net::InputBatch& batch) {
    for (std::size_t i = 0; i < batch.commands.size(); ++i) {
        const net::Tick tick = batch.tickAt(i);
        if (!isAcceptable(tick)) {
            continue;
        }
        _pending.push_back(Entry{tick, batch.commands[i]});
        _newestReceived = tick;
    }
    while (_pending.size() > kMaxBacklog) {
        _pending.pop_front();
    }
}

bool InputQueue::isAcceptable(net::Tick tick) const {
    if (tick == 0) {
        return false; // 0 veut dire « aucun input appliqué » : les ticks commencent à 1
    }
    if (!_newestReceived) {
        return true; // premier input : le client choisit son point de départ
    }
    // Ni déjà reçu (redondance, ou trop vieux), ni un saut invraisemblable.
    return tick > *_newestReceived && tick - *_newestReceived <= kMaxTickJump;
}

std::optional<InputQueue::Entry> InputQueue::pop() {
    if (_pending.empty()) {
        return std::nullopt;
    }
    Entry entry = _pending.front();
    _pending.pop_front();
    return entry;
}

} // namespace odyssee::server
