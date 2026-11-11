#include "odyssee/net/Session.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/Acknowledgements.hpp"

#include <utility>

namespace odyssee::net {

namespace {

constexpr std::size_t kKindSize = 1;

/// Place laissée aux messages fiables : le paquet entier, moins l'en-tête, le type, le jeton de
/// session et le compteur des non fiables (le compteur des fiables est compté par ReliableSender).
constexpr std::size_t kReliableBudget =
    kMaxPacketSize - PacketHeader::kSize - kKindSize - game_packet::kSessionTokenSize -
    game_packet::kUnreliableListOverhead;

/// Poids d'une nouvelle mesure dans la moyenne glissante de l'aller-retour (comme TCP : 1/8).
constexpr int kRoundTripSmoothing = 8;

} // namespace

Session::Session(SessionToken sessionToken, std::chrono::milliseconds resendDelay)
    : _token(sessionToken), _reliableOut(resendDelay) {}

bool Session::queueReliable(std::vector<std::byte> payload) {
    return _reliableOut.enqueue(std::move(payload)).has_value();
}

void Session::queueUnreliable(const GameMessage& message) {
    _unreliableOut.push_back(
        UnreliableMessage{static_cast<std::uint8_t>(typeOf(message)), encodeGameMessage(message)});
}

std::vector<std::byte> Session::buildPacket(Clock::time_point now) {
    const Sequence sequence = _nextSequence++;
    const PacketHeader header{kProtocolVersion, sequence, _received.ack(), _received.ackBits()};

    GamePacketBody body;
    body.sessionToken = _token;
    body.reliable = _reliableOut.collectForPacket(sequence, now, kReliableBudget);
    appendUnreliable(body);

    _sentAt[sequence % _sentAt.size()] = SentPacket{sequence, now};
    ++_stats.packetsSent;
    return game_packet::build(header, body);
}

void Session::appendUnreliable(GamePacketBody& body) {
    std::size_t size = game_packet::packetSize(body);
    for (UnreliableMessage& message : _unreliableOut) {
        const bool fits = size + game_packet::encodedSize(message) <= kMaxPacketSize &&
                          body.unreliable.size() < game_packet::kMaxUnreliablePerPacket;
        if (!fits) {
            ++_stats.unreliableDropped;
            continue;
        }
        size += game_packet::encodedSize(message);
        body.unreliable.push_back(std::move(message));
    }
    _unreliableOut.clear();
}

bool Session::receive(const IncomingPacket& packet, Clock::time_point now) {
    // Le type et le jeton d'abord : un paquet qui ne vient pas de l'autre côté de la connexion
    // n'est même pas analysé.
    auto body = isFromPeer(packet) ? game_packet::parse(packet.body) : std::nullopt;
    // Le numéro n'est enregistré qu'une fois le paquet jugé valide : un paquet forgé ne doit pas
    // pouvoir faire « acquitter » des numéros jamais reçus.
    if (!body || _received.record(packet.header.sequence) != Arrival::New) {
        ++_stats.packetsIgnored;
        return false;
    }

    ++_stats.packetsReceived;
    onAcknowledged(packet.header, now);
    acceptMessages(*body);
    return true;
}

bool Session::isFromPeer(const IncomingPacket& packet) const {
    return packet.kind == PacketKind::Game && packet.header.hasCurrentProtocol() &&
           sessionTokenOf(packet) == _token;
}

void Session::onAcknowledged(const PacketHeader& header, Clock::time_point now) {
    const auto acknowledged = acknowledgedSequences(header.ack, header.ackBits);
    _reliableOut.onAcknowledged(acknowledged);
    if (!acknowledged.empty()) {
        sampleRoundTrip(acknowledged.front(), now); // le plus récent : la meilleure mesure
    }
}

void Session::sampleRoundTrip(Sequence acknowledged, Clock::time_point now) {
    auto& sent = _sentAt[acknowledged % _sentAt.size()];
    if (!sent || sent->sequence != acknowledged) {
        return; // déjà mesuré, ou trop ancien (case réutilisée)
    }
    const Clock::duration sample = now - sent->at;
    sent.reset(); // un paquet n'est mesuré qu'une fois (il est acquitté par plusieurs paquets)
    _roundTrip = _roundTrip ? *_roundTrip + (sample - *_roundTrip) / kRoundTripSmoothing : sample;
}

void Session::acceptMessages(GamePacketBody& body) {
    for (ReliableMessage& message : body.reliable) {
        _reliableIn.receive(std::move(message));
    }
    for (const UnreliableMessage& message : body.unreliable) {
        if (_acceptedType && message.type != static_cast<std::uint8_t>(*_acceptedType)) {
            continue; // type que ce côté n'attend pas : ignoré avant décodage
        }
        if (auto decoded = decodeGameMessage(message.type, message.data)) {
            _messagesIn.push_back(std::move(*decoded));
        }
    }
    if (_messagesIn.size() > kMaxQueuedMessages) {
        const auto excess = static_cast<std::ptrdiff_t>(_messagesIn.size() - kMaxQueuedMessages);
        _messagesIn.erase(_messagesIn.begin(), _messagesIn.begin() + excess);
    }
}

std::vector<GameMessage> Session::takeMessages() { return std::exchange(_messagesIn, {}); }

std::vector<std::vector<std::byte>> Session::takeReliable() {
    return _reliableIn.takeDeliverable();
}

} // namespace odyssee::net
