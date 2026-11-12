#include "odyssee/client/TrafficMeter.hpp"

#include <algorithm>
#include <set>

namespace odyssee::client {

namespace {

template <typename Events>
double kilobytesPerSecond(const Events& events, TrafficMeter::Clock::time_point now) {
    std::size_t bytes = 0;
    for (const auto& event : events) {
        if (now - event.at <= TrafficMeter::kWindow) {
            bytes += event.bytes;
        }
    }
    const double seconds =
        std::chrono::duration<double>(TrafficMeter::kWindow).count();
    return static_cast<double>(bytes) / 1024.0 / seconds;
}

} // namespace

void TrafficMeter::onSent(std::size_t bytes, Clock::time_point now) {
    _sent.push_back({now, bytes});
    forgetOld(now);
}

void TrafficMeter::onReceived(std::size_t bytes, Clock::time_point now) {
    _received.push_back({now, bytes});
    forgetOld(now);
}

void TrafficMeter::onGamePacket(net::Sequence sequence, Clock::time_point now) {
    _arrivals.push_back({now, sequence});
    forgetOld(now);
}

TrafficStats TrafficMeter::stats(Clock::time_point now) const {
    return {kilobytesPerSecond(_sent, now), kilobytesPerSecond(_received, now), lossPercent(now)};
}

void TrafficMeter::forgetOld(Clock::time_point now) {
    const auto old = [now](const auto& event) { return now - event.at > kWindow; };
    while (!_sent.empty() && old(_sent.front())) {
        _sent.pop_front();
    }
    while (!_received.empty() && old(_received.front())) {
        _received.pop_front();
    }
    while (!_arrivals.empty() && old(_arrivals.front())) {
        _arrivals.pop_front();
    }
}

double TrafficMeter::lossPercent(Clock::time_point now) const {
    // Numéros reçus sur la fenêtre, rapportés au plus ancien (le rebouclage à 65536 est géré par
    // la distance modulo, la fenêtre étant bien plus courte qu'un demi-tour).
    std::set<std::uint16_t> offsets;
    std::optional<net::Sequence> oldest;
    for (const Arrival& arrival : _arrivals) {
        if (now - arrival.at > kWindow) {
            continue;
        }
        if (!oldest || net::isMoreRecent(*oldest, arrival.sequence)) {
            oldest = arrival.sequence;
        }
    }
    if (!oldest) {
        return 0.0;
    }
    for (const Arrival& arrival : _arrivals) {
        if (now - arrival.at <= kWindow) {
            offsets.insert(net::sequenceDistance(arrival.sequence, *oldest));
        }
    }
    const double expected = static_cast<double>(*offsets.rbegin()) + 1.0;
    return std::max(0.0, 100.0 * (1.0 - static_cast<double>(offsets.size()) / expected));
}

} // namespace odyssee::client
