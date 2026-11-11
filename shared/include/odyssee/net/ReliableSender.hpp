#pragma once

#include "odyssee/net/ReliableMessage.hpp"
#include "odyssee/net/Sequence.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace odyssee::net {

/// Côté envoi du canal fiable : garde chaque message jusqu'à ce qu'un paquet qui le transportait
/// soit acquitté, et le renvoie s'il tarde à l'être.
///
/// Le temps est passé en paramètre (`now`) : les tests peuvent « avancer l'horloge » sans attendre.
class ReliableSender {
  public:
    using Clock = std::chrono::steady_clock;

    explicit ReliableSender(std::chrono::milliseconds resendDelay = std::chrono::milliseconds{100});

    /// Messages en attente d'acquittement, au plus : au-delà, enqueue() refuse (l'autre côté ne
    /// répond plus, ou le jeu envoie plus vite que le réseau ne débite). Borne aussi la mémoire,
    /// et garde les numéros de message (16 bits) sans ambiguïté.
    static constexpr std::size_t kMaxPending = 4096;

    /// Fenêtre d'envoi : seuls les messages à moins de kSendWindow numéros du plus ancien non
    /// acquitté partent. Plus petite que ReliableReceiver::kMaxAhead : le récepteur ne refuse
    /// jamais un message que l'émetteur a le droit d'envoyer.
    static constexpr std::uint16_t kSendWindow = 512;

    /// Met un message en file d'envoi. Renvoie std::nullopt si `payload` ne tiendrait jamais
    /// dans un paquet, ou si kMaxPending messages attendent déjà.
    [[nodiscard]] std::optional<MessageId> enqueue(std::vector<std::byte> payload);

    /// Choisit les messages à mettre dans le paquet `sequence` (jamais envoyés, ou à renvoyer),
    /// dans la limite de `budget` octets, et retient qu'ils voyagent dans ce paquet.
    [[nodiscard]] std::vector<ReliableMessage> collectForPacket(Sequence sequence,
                                                                Clock::time_point now,
                                                                std::size_t budget);

    /// Prend en compte les paquets que l'autre côté confirme avoir reçus.
    void onAcknowledged(std::span<const Sequence> sequences);

    /// Messages pas encore confirmés.
    [[nodiscard]] std::size_t pendingCount() const { return _pending.size(); }

    /// Taille maximale du contenu d'un message (il doit tenir seul dans un paquet).
    static std::size_t maxPayloadSize();

  private:
    struct Pending {
        ReliableMessage message;
        std::optional<Clock::time_point> lastSent;
    };

    [[nodiscard]] bool isDue(const Pending& pending, Clock::time_point now) const;
    [[nodiscard]] bool isInSendWindow(const Pending& pending) const;
    void confirm(MessageId id);
    void forgetUnacknowledgeablePackets(Sequence current);

    std::chrono::milliseconds _resendDelay;
    MessageId _nextId = 0;
    std::deque<Pending> _pending; ///< dans l'ordre d'envoi
    /// Pour chaque paquet envoyé : les messages qu'il transportait.
    std::unordered_map<Sequence, std::vector<MessageId>> _carriedBy;
};

} // namespace odyssee::net
