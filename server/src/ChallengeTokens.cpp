#include "ChallengeTokens.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"

namespace odyssee::server {

namespace {

/// Numéro de la période de validité contenant `now` (une période = kHandshakeTimeout).
std::int64_t periodOf(ChallengeTokens::Clock::time_point now) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    return static_cast<std::int64_t>(elapsed.count() / kHandshakeTimeout.count());
}

} // namespace

ChallengeTokens::ChallengeTokens(const SipKey& key) : _key(key) {}

std::uint64_t ChallengeTokens::issue(const Claim& claim, Clock::time_point now) const {
    return tokenFor(claim, periodOf(now));
}

bool ChallengeTokens::verify(const Claim& claim, std::uint64_t token,
                             Clock::time_point now) const {
    // Période en cours ou précédente : un défi émis juste avant un changement de période reste
    // valable le temps d'une poignée de main.
    const std::int64_t period = periodOf(now);
    return token == tokenFor(claim, period) || token == tokenFor(claim, period - 1);
}

std::uint64_t ChallengeTokens::tokenFor(const Claim& claim, std::int64_t period) const {
    net::ByteWriter writer;
    writer.writeU32(claim.address.ip);
    writer.writeU16(claim.address.port);
    writer.writeU64(claim.clientSalt);
    writer.writeU64(static_cast<std::uint64_t>(period));
    writer.writeString(claim.playerName);
    return sipHash24(_key, writer.data());
}

} // namespace odyssee::server
