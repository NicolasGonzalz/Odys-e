#include "odyssee/net/ReliableSender.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/GamePacket.hpp"
#include "odyssee/net/PacketHeader.hpp"
#include "odyssee/net/ReliableCodec.hpp"

#include <algorithm>
#include <iterator>

namespace odyssee::net {

namespace {

/// Au-delà de 32 paquets d'écart, un paquet ne peut plus être acquitté (fenêtre de ackBits) :
/// ses messages, s'ils ne sont pas confirmés, seront simplement renvoyés.
constexpr std::uint16_t kAckWindow = 32;

} // namespace

ReliableSender::ReliableSender(std::chrono::milliseconds resendDelay) : _resendDelay(resendDelay) {}

std::size_t ReliableSender::maxPayloadSize() {
    // Un message seul doit tenir dans un paquet de jeu vide : en-tête, type, jeton, liste des fiables
    // (ce message), liste des non fiables (vide).
    constexpr std::size_t kKindSize = 1;
    return kMaxPacketSize - PacketHeader::kSize - kKindSize - game_packet::kSessionTokenSize -
           reliable_codec::kListOverhead -
           reliable_codec::kMessageOverhead - game_packet::kUnreliableListOverhead;
}

std::optional<MessageId> ReliableSender::enqueue(std::vector<std::byte> payload) {
    if (payload.size() > maxPayloadSize()) {
        return std::nullopt;
    }
    const MessageId id = _nextId++;
    _pending.push_back({ReliableMessage{id, std::move(payload)}, std::nullopt});
    return id;
}

std::vector<ReliableMessage> ReliableSender::collectForPacket(Sequence sequence,
                                                              Clock::time_point now,
                                                              std::size_t budget) {
    forgetUnacknowledgeablePackets(sequence);

    std::vector<ReliableMessage> selected;
    std::size_t used = reliable_codec::kListOverhead;

    for (Pending& pending : _pending) {
        if (!isDue(pending, now)) {
            continue; // envoyé récemment : on attend encore son acquittement
        }

        const std::size_t size = reliable_codec::encodedSize(pending.message);
        const bool full = used + size > budget ||
                          selected.size() == reliable_codec::kMaxMessagesPerPacket;
        if (full) {
            break; // on garde l'ordre : les suivants partiront dans un prochain paquet
        }

        selected.push_back(pending.message);
        pending.lastSent = now;
        used += size;
    }

    if (!selected.empty()) {
        auto& carried = _carriedBy[sequence];
        for (const ReliableMessage& message : selected) {
            carried.push_back(message.id);
        }
    }
    return selected;
}

void ReliableSender::onAcknowledged(std::span<const Sequence> sequences) {
    for (const Sequence sequence : sequences) {
        const auto carried = _carriedBy.find(sequence);
        if (carried == _carriedBy.end()) {
            continue; // paquet inconnu ou déjà traité
        }
        for (const MessageId id : carried->second) {
            confirm(id);
        }
        _carriedBy.erase(carried);
    }
}

bool ReliableSender::isDue(const Pending& pending, Clock::time_point now) const {
    return !pending.lastSent || now - *pending.lastSent >= _resendDelay;
}

void ReliableSender::confirm(MessageId id) {
    std::erase_if(_pending, [id](const Pending& pending) { return pending.message.id == id; });
}

void ReliableSender::forgetUnacknowledgeablePackets(Sequence current) {
    std::erase_if(_carriedBy, [current](const auto& entry) {
        return sequenceDistance(current, entry.first) > kAckWindow;
    });
}

} // namespace odyssee::net
