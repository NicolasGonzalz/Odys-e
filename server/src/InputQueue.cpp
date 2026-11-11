#include "InputQueue.hpp"

namespace odyssee::server {

void InputQueue::push(const net::InputBatch& batch) {
    for (std::size_t i = 0; i < batch.commands.size(); ++i) {
        const net::Tick tick = batch.tickAt(i);
        if (_newestReceived && tick <= *_newestReceived) {
            continue; // déjà reçu dans un paquet précédent (redondance) ou trop vieux
        }
        _pending.push_back(Entry{tick, batch.commands[i]});
        _newestReceived = tick;
    }
    while (_pending.size() > kMaxBacklog) {
        _pending.pop_front();
    }
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
