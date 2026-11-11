#include "SipHash.hpp"

#include <bit>
#include <random>

namespace odyssee::server {

namespace {

struct State {
    std::uint64_t v0, v1, v2, v3;

    void round() {
        v0 += v1;
        v1 = std::rotl(v1, 13) ^ v0;
        v0 = std::rotl(v0, 32);
        v2 += v3;
        v3 = std::rotl(v3, 16) ^ v2;
        v0 += v3;
        v3 = std::rotl(v3, 21) ^ v0;
        v2 += v1;
        v1 = std::rotl(v1, 17) ^ v2;
        v2 = std::rotl(v2, 32);
    }

    void absorb(std::uint64_t word) {
        v3 ^= word;
        round();
        round();
        v0 ^= word;
    }
};

/// Jusqu'à 8 octets lus en little-endian (les octets manquants valent 0).
std::uint64_t readWord(std::span<const std::byte> bytes) {
    std::uint64_t word = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        word |= std::uint64_t{std::to_integer<std::uint8_t>(bytes[i])} << (8 * i);
    }
    return word;
}

/// Étape de splitmix64 : étale une graine sur 64 bits bien mélangés.
std::uint64_t splitMix(std::uint64_t& state) {
    state += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

std::uint64_t randomWord(std::random_device& device) {
    return (std::uint64_t{device()} << 32) | std::uint64_t{device()};
}

} // namespace

SipKey SipKey::fromSeed(std::uint64_t seed) {
    const std::uint64_t k0 = splitMix(seed);
    const std::uint64_t k1 = splitMix(seed);
    return SipKey{k0, k1};
}

SipKey SipKey::random() {
    std::random_device device;
    const std::uint64_t k0 = randomWord(device);
    const std::uint64_t k1 = randomWord(device);
    return SipKey{k0, k1};
}

std::uint64_t sipHash24(const SipKey& key, std::span<const std::byte> data) {
    State s{key.k0 ^ 0x736F6D6570736575ULL, key.k1 ^ 0x646F72616E646F6DULL,
            key.k0 ^ 0x6C7967656E657261ULL, key.k1 ^ 0x7465646279746573ULL};

    const std::size_t fullWords = data.size() / 8;
    for (std::size_t i = 0; i < fullWords; ++i) {
        s.absorb(readWord(data.subspan(i * 8, 8)));
    }
    // Dernier mot : les octets restants, et la longueur du message dans l'octet de poids fort.
    const std::uint64_t tail = readWord(data.subspan(fullWords * 8));
    s.absorb(tail | (static_cast<std::uint64_t>(data.size() & 0xFF) << 56));

    s.v2 ^= 0xFF;
    for (int i = 0; i < 4; ++i) {
        s.round();
    }
    return s.v0 ^ s.v1 ^ s.v2 ^ s.v3;
}

} // namespace odyssee::server
