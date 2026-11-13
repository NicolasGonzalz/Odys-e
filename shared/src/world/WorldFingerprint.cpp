#include "odyssee/world/WorldFingerprint.hpp"

namespace odyssee::world {

std::uint64_t fingerprintOf(std::span<const std::byte> bytes) {
    constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t hash = kOffsetBasis;
    for (const std::byte byte : bytes) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= kPrime;
    }
    // 0 est réservé au « pas de carte » : une carte ne doit jamais tomber dessus.
    return hash == kNoMapFingerprint ? 1 : hash;
}

} // namespace odyssee::world
