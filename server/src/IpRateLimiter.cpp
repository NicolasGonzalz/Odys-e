#include "IpRateLimiter.hpp"

#include <iterator>

namespace odyssee::server {

namespace {

/// Le ménage parcourt toute la table : pas plus souvent que ça, même sous un flot d'adresses.
constexpr std::chrono::milliseconds kCleanupInterval{250};

} // namespace

IpRateLimiter::IpRateLimiter(TokenBucket::Rate rate, std::size_t maxTracked)
    : _rate(rate), _maxTracked(maxTracked) {}

bool IpRateLimiter::allow(std::uint32_t ip, Clock::time_point now) {
    if (const auto known = _buckets.find(ip); known != _buckets.end()) {
        return known->second.take(now);
    }
    if (_buckets.size() >= _maxTracked) {
        forgetIdle(now);
    }
    if (_buckets.size() >= _maxTracked) {
        return false; // table pleine d'adresses actives : on ne suit plus de nouvelle adresse
    }
    return _buckets.emplace(ip, TokenBucket(_rate, now)).first->second.take(now);
}

void IpRateLimiter::forgetIdle(Clock::time_point now) {
    if (_hasCleaned && now - _lastCleanup < kCleanupInterval) {
        return;
    }
    _hasCleaned = true;
    _lastCleanup = now;
    for (auto bucket = _buckets.begin(); bucket != _buckets.end();) {
        bucket = bucket->second.isFull(now) ? _buckets.erase(bucket) : std::next(bucket);
    }
}

} // namespace odyssee::server
