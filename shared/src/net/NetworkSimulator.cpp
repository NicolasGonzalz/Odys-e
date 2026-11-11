#include "odyssee/net/NetworkSimulator.hpp"

#include <algorithm>
#include <cmath>

namespace odyssee::net {

namespace {

NetworkConditions sanitized(NetworkConditions conditions) {
    using std::chrono::milliseconds;
    conditions.latency = std::max(conditions.latency, milliseconds{0});
    conditions.jitter = std::max(conditions.jitter, milliseconds{0});
    const auto percent = [](double value) {
        return std::isnan(value) ? 0.0 : std::clamp(value, 0.0, 100.0);
    };
    conditions.lossPercent = percent(conditions.lossPercent);
    conditions.duplicatePercent = percent(conditions.duplicatePercent);
    return conditions;
}

} // namespace

bool NetworkConditions::isPerfect() const {
    return latency.count() <= 0 && jitter.count() <= 0 && lossPercent <= 0.0 &&
           duplicatePercent <= 0.0;
}

NetworkSimulator::NetworkSimulator(NetworkConditions conditions, std::uint64_t seed)
    : _conditions(sanitized(conditions)), _random(seed) {}

void NetworkSimulator::submit(const Address& to, std::span<const std::byte> bytes,
                              Clock::time_point now) {
    ++_stats.submitted;
    if (roll(_conditions.lossPercent)) {
        ++_stats.dropped;
        return;
    }
    schedule(to, bytes, now);
    if (roll(_conditions.duplicatePercent)) {
        ++_stats.duplicated;
        schedule(to, bytes, now); // la copie a son propre délai : elle peut arriver avant
    }
}

std::vector<NetworkSimulator::Datagram> NetworkSimulator::takeDue(Clock::time_point now) {
    std::vector<Datagram> due;
    while (!_scheduled.empty() && _scheduled.begin()->first.first <= now) {
        due.push_back(std::move(_scheduled.begin()->second));
        _scheduled.erase(_scheduled.begin());
    }
    return due;
}

std::optional<NetworkSimulator::Clock::time_point> NetworkSimulator::nextDue() const {
    if (_scheduled.empty()) {
        return std::nullopt;
    }
    return _scheduled.begin()->first.first;
}

bool NetworkSimulator::roll(double percent) {
    if (percent <= 0.0) {
        return false; // pas de tirage : 0 % ne consomme pas le hasard
    }
    // 53 bits du générateur → réel uniforme dans [0, 1), sans distribution de la bibliothèque
    // standard (leur algorithme change d'une plateforme à l'autre).
    const double unit = static_cast<double>(_random() >> 11) * 0x1.0p-53;
    return unit * 100.0 < percent;
}

NetworkSimulator::Clock::duration NetworkSimulator::randomDelay() {
    const auto jitter = _conditions.jitter.count();
    if (jitter <= 0) {
        return _conditions.latency;
    }
    const auto span = static_cast<std::uint64_t>(2 * jitter + 1);
    const auto offset = static_cast<std::int64_t>(_random() % span) - jitter;
    const auto delay = _conditions.latency + std::chrono::milliseconds{offset};
    return std::max<Clock::duration>(delay, Clock::duration::zero());
}

void NetworkSimulator::schedule(const Address& to, std::span<const std::byte> bytes,
                                Clock::time_point now) {
    const Key key{now + randomDelay(), _nextOrder++};
    _scheduled.emplace(key, Datagram{to, {bytes.begin(), bytes.end()}});
}

} // namespace odyssee::net
