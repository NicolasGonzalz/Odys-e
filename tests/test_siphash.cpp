#include "ChallengeTokens.hpp"
#include "SipHash.hpp"

#include "odyssee/core/Config.hpp"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <vector>

using namespace odyssee;
using odyssee::server::ChallengeTokens;
using odyssee::server::SipKey;
using odyssee::server::sipHash24;
using namespace std::chrono_literals;

namespace {

/// Les `count` premiers octets 00 01 02… (les messages des vecteurs de référence).
std::vector<std::byte> countingBytes(std::size_t count) {
    std::vector<std::byte> bytes(count);
    for (std::size_t i = 0; i < count; ++i) {
        bytes[i] = static_cast<std::byte>(i);
    }
    return bytes;
}

} // namespace

TEST_CASE("SipHash-2-4 : vecteurs de référence (clé 00..0f, message 00..n-1)") {
    // Valeurs de l'implémentation de référence (Aumasson & Bernstein, vectors.h).
    const SipKey key{0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL};
    CHECK(sipHash24(key, countingBytes(0)) == 0x726FDB47DD0E0E31ULL);
    CHECK(sipHash24(key, countingBytes(1)) == 0x74F839C593DC67FDULL);
    CHECK(sipHash24(key, countingBytes(7)) == 0xAB0200F58B01D137ULL);
    CHECK(sipHash24(key, countingBytes(8)) == 0x93F5F5799A932462ULL);
    CHECK(sipHash24(key, countingBytes(15)) == 0xA129CA6149BE45E5ULL);
}

TEST_CASE("SipKey : une graine donne toujours la même clé, le tirage système non") {
    const SipKey a = SipKey::fromSeed(7);
    const SipKey b = SipKey::fromSeed(7);
    CHECK(a.k0 == b.k0);
    CHECK(a.k1 == b.k1);
    CHECK(a.k0 != SipKey::fromSeed(8).k0);

    const SipKey r1 = SipKey::random();
    const SipKey r2 = SipKey::random();
    CHECK((r1.k0 != r2.k0 || r1.k1 != r2.k1));
}

TEST_CASE("ChallengeTokens : valable pour la demande exacte, 5 à 10 s") {
    const ChallengeTokens tokens(SipKey::fromSeed(1));
    const ChallengeTokens::Clock::time_point t0{};
    const ChallengeTokens::Claim claim{net::Address::loopback(5000), 42, "Teo"};
    const auto token = tokens.issue(claim, t0 + 4s);

    CHECK(tokens.verify(claim, token, t0 + 4s));
    CHECK(tokens.verify(claim, token, t0 + 9s));         // période suivante : encore bon
    CHECK_FALSE(tokens.verify(claim, token, t0 + 10s));  // deux périodes plus tard : périmé
    CHECK_FALSE(tokens.verify(claim, token + 1, t0 + 4s));

    CHECK_FALSE(tokens.verify({net::Address::loopback(5001), 42, "Teo"}, token, t0 + 4s));
    CHECK_FALSE(tokens.verify({net::Address::loopback(5000), 43, "Teo"}, token, t0 + 4s));
    CHECK_FALSE(tokens.verify({net::Address::loopback(5000), 42, "Lea"}, token, t0 + 4s));

    const ChallengeTokens other(SipKey::fromSeed(2));
    CHECK_FALSE(other.verify(claim, token, t0 + 4s));
}
