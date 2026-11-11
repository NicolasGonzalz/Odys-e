#include "odyssee/client/InputHistory.hpp"

#include <algorithm>

namespace odyssee::client {

void InputHistory::record(net::Tick tick, const net::InputCommand& command) {
    if (_commands.empty() || tick != lastTick() + 1) {
        _commands.clear();
        _firstTick = tick;
    }

    _commands.push_back(command);

    if (_commands.size() > kCapacity) {
        _commands.pop_front();
        ++_firstTick;
    }
}

net::InputBatch InputHistory::recentBatch(std::size_t count) const {
    const std::size_t taken = std::min(count, _commands.size());
    if (taken == 0) {
        return {};
    }

    const auto first = _commands.end() - static_cast<std::ptrdiff_t>(taken);
    return {lastTick() - static_cast<net::Tick>(taken - 1), {first, _commands.end()}};
}

bool InputHistory::covers(net::Tick tick) const {
    return !_commands.empty() && tick + 1 >= _firstTick && tick <= lastTick();
}

std::vector<net::InputCommand> InputHistory::after(net::Tick tick) const {
    if (!covers(tick)) {
        return {};
    }
    const auto skipped = static_cast<std::ptrdiff_t>(tick + 1 - _firstTick);
    return {_commands.begin() + skipped, _commands.end()};
}

net::Tick InputHistory::lastTick() const {
    if (_commands.empty()) {
        return 0;
    }
    return _firstTick + static_cast<net::Tick>(_commands.size() - 1);
}

} // namespace odyssee::client
