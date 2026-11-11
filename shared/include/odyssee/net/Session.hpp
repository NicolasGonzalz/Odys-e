#pragma once

#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/net/GamePacket.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/ReceivedPackets.hpp"
#include "odyssee/net/ReliableReceiver.hpp"
#include "odyssee/net/ReliableSender.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace odyssee::net {

/// Échange de paquets de jeu avec l'autre côté, une fois connecté : le serveur en a une par
/// joueur, le client une seule. Numérote les paquets, acquitte ceux reçus, fait circuler les
/// messages fiables (renvoyés jusqu'à acquittement) et non fiables, et mesure l'aller-retour.
///
/// Sans réseau, comme ClientConnection : l'appelant envoie les octets de buildPacket() et donne
/// à receive() les paquets de jeu reçus. Le temps est passé en paramètre (`now`).
class Session {
  public:
    using Clock = std::chrono::steady_clock;

    struct Stats {
        std::uint64_t packetsSent = 0;
        std::uint64_t packetsReceived = 0;   ///< nouveaux et valides
        std::uint64_t packetsIgnored = 0;    ///< doublons, trop vieux, ou invalides
        std::uint64_t unreliableDropped = 0; ///< non fiables qui ne tenaient pas dans le paquet
    };

    /// `sessionToken` : jeton convenu à la poignée de main (ClientConnection::sessionToken()
    /// côté client). Il est écrit dans chaque paquet envoyé et exigé de chaque paquet reçu.
    explicit Session(SessionToken sessionToken = 0,
                     std::chrono::milliseconds resendDelay = std::chrono::milliseconds{100});

    /// N'accepte plus que les messages non fiables de ce type : les autres sont ignorés **sans
    /// être décodés**. Le serveur n'attend que des inputs : un Snapshot reçu d'un client est faux.
    void acceptOnly(GameMessageType type) { _acceptedType = type; }

    /// Message fiable : livré une fois, dans l'ordre. Faux s'il ne tiendrait dans aucun paquet,
    /// ou si trop de messages attendent déjà leur acquittement (ReliableSender::kMaxPending).
    [[nodiscard]] bool queueReliable(std::vector<std::byte> payload);

    /// Message non fiable pour le prochain paquet.
    void queueUnreliable(const GameMessage& message);

    /// Prochain paquet à envoyer (toujours un, même vide : il acquitte ce qui a été reçu). Les
    /// messages fiables dus passent d'abord ; les non fiables qui ne tiennent plus sont abandonnés
    /// (au paquet suivant, ils seraient périmés).
    [[nodiscard]] std::vector<std::byte> buildPacket(Clock::time_point now);

    /// Prend en compte un paquet de jeu (déjà découpé par parsePacket). Faux s'il est ignoré :
    /// mauvais jeton de session, doublon, trop vieux ou contenu invalide. Un message non fiable
    /// invalide est ignoré seul.
    bool receive(const IncomingPacket& packet, Clock::time_point now);

    /// Messages non fiables reçus depuis le dernier appel, dans l'ordre d'arrivée (au plus
    /// kMaxQueuedMessages : au-delà, les plus anciens sont oubliés).
    [[nodiscard]] std::vector<GameMessage> takeMessages();

    /// Contenus fiables arrivés, dans l'ordre d'envoi.
    [[nodiscard]] std::vector<std::vector<std::byte>> takeReliable();

    /// Aller-retour estimé (moyenne glissante), dès qu'un paquet a été acquitté.
    [[nodiscard]] std::optional<Clock::duration> roundTripTime() const { return _roundTrip; }

    [[nodiscard]] const Stats& stats() const { return _stats; }

    static constexpr std::size_t kMaxQueuedMessages = 64;

  private:
    struct SentPacket {
        Sequence sequence = 0;
        Clock::time_point at;
    };

    void appendUnreliable(GamePacketBody& body);
    void onAcknowledged(const PacketHeader& header, Clock::time_point now);
    void sampleRoundTrip(Sequence acknowledged, Clock::time_point now);
    void acceptMessages(GamePacketBody& body);

    [[nodiscard]] bool isFromPeer(const IncomingPacket& packet) const;

    SessionToken _token = 0;
    std::optional<GameMessageType> _acceptedType; ///< absent : tous les types connus
    Sequence _nextSequence = 0;
    ReceivedPackets _received;
    ReliableSender _reliableOut;
    ReliableReceiver _reliableIn;
    std::vector<UnreliableMessage> _unreliableOut;
    std::vector<GameMessage> _messagesIn;

    /// Heures d'envoi des derniers paquets (au-delà de 64 paquets, un acquittement ne peut plus
    /// arriver : la fenêtre de ackBits en couvre 33).
    std::array<std::optional<SentPacket>, 64> _sentAt{};
    std::optional<Clock::duration> _roundTrip;
    Stats _stats;
};

} // namespace odyssee::net
