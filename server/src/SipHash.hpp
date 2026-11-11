#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace odyssee::server {

/// Clé secrète de 128 bits pour SipHash.
struct SipKey {
    std::uint64_t k0 = 0;
    std::uint64_t k1 = 0;

    /// Clé reproductible dérivée d'une graine : pour les tests uniquement.
    [[nodiscard]] static SipKey fromSeed(std::uint64_t seed);

    /// Clé imprévisible, tirée du générateur du système (std::random_device).
    [[nodiscard]] static SipKey random();
};

/// SipHash-2-4 (Aumasson & Bernstein) : code d'authentification de message sur 64 bits.
/// Sans la clé, la sortie est imprévisible, même en ayant observé beaucoup d'autres sorties :
/// c'est ce qui manque à un générateur comme mt19937_64, dont l'état se reconstruit.
[[nodiscard]] std::uint64_t sipHash24(const SipKey& key, std::span<const std::byte> data);

} // namespace odyssee::server
