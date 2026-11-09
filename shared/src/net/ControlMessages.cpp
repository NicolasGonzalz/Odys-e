#include "odyssee/net/ControlMessages.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/PacketFraming.hpp"

namespace odyssee::net {

namespace {

/// On accepte de lire un pseudo plus long que permis, pour pouvoir répondre « pseudo invalide »
/// au lieu d'ignorer la demande. La règle elle-même est dans PlayerName.
constexpr std::size_t kMaxNameOnWire = 64;

// --- Écriture : une fonction par message ------------------------------------------------------

void writeBody(ByteWriter& w, const ConnectRequest& m) { w.writeString(m.playerName); }
void writeBody(ByteWriter& w, const Challenge& m) { w.writeU64(m.token); }
void writeBody(ByteWriter& w, const ChallengeResponse& m) { w.writeU64(m.token); }
void writeBody(ByteWriter& w, const Accepted& m) { w.writeU8(m.playerId); }
void writeBody(ByteWriter& w, const Refused& m) { w.writeU8(static_cast<std::uint8_t>(m.reason)); }
void writeBody(ByteWriter&, const Disconnect&) {}
void writeBody(ByteWriter&, const KeepAlive&) {}

// --- Lecture : une fonction par message -------------------------------------------------------

std::optional<ControlMessage> readConnectRequest(ByteReader& r) {
    auto name = r.readString(kMaxNameOnWire);
    if (!name) {
        return std::nullopt;
    }
    return ConnectRequest{std::move(*name)};
}

template <typename Message> std::optional<ControlMessage> readToken(ByteReader& r) {
    const auto token = r.readU64();
    if (!token) {
        return std::nullopt;
    }
    return Message{*token};
}

std::optional<ControlMessage> readAccepted(ByteReader& r) {
    const auto id = r.readU8();
    if (!id || *id >= kMaxPlayers) {
        return std::nullopt;
    }
    return Accepted{*id};
}

std::optional<ControlMessage> readRefused(ByteReader& r) {
    const auto reason = r.readU8();
    const bool known = reason && *reason >= 1 && *reason <= 3;
    if (!known) {
        return std::nullopt;
    }
    return Refused{static_cast<RefuseReason>(*reason)};
}

std::optional<ControlMessage> readMessage(PacketKind kind, ByteReader& r) {
    switch (kind) {
    case PacketKind::ConnectRequest:
        return readConnectRequest(r);
    case PacketKind::Challenge:
        return readToken<Challenge>(r);
    case PacketKind::ChallengeResponse:
        return readToken<ChallengeResponse>(r);
    case PacketKind::Accepted:
        return readAccepted(r);
    case PacketKind::Refused:
        return readRefused(r);
    case PacketKind::Disconnect:
        return Disconnect{};
    case PacketKind::KeepAlive:
        return KeepAlive{};
    }
    return std::nullopt;
}

} // namespace

PacketKind kindOf(const ControlMessage& message) {
    // L'ordre des types du variant suit les valeurs de PacketKind (ConnectRequest = 1, …).
    return static_cast<PacketKind>(message.index() + 1);
}

std::vector<std::byte> encodeBody(const ControlMessage& message) {
    ByteWriter writer;
    std::visit([&writer](const auto& m) { writeBody(writer, m); }, message);
    return {writer.data().begin(), writer.data().end()};
}

std::optional<ControlMessage> decodeControl(PacketKind kind, std::span<const std::byte> body) {
    ByteReader reader(body);
    auto message = readMessage(kind, reader);
    if (!message || reader.remaining() != 0) {
        return std::nullopt; // contenu invalide, ou octets en trop
    }
    return message;
}

std::vector<std::byte> buildControlPacket(const ControlMessage& message) {
    // Les messages de connexion n'utilisent pas les numéros de séquence (ils viennent après).
    const PacketHeader header{kProtocolVersion, 0, 0, 0};
    return buildPacket(header, kindOf(message), encodeBody(message));
}

std::string_view describe(RefuseReason reason) {
    switch (reason) {
    case RefuseReason::WrongVersion:
        return "version du jeu différente de celle du serveur";
    case RefuseReason::ServerFull:
        return "serveur plein";
    case RefuseReason::InvalidName:
        return "pseudo invalide";
    }
    return "raison inconnue";
}

} // namespace odyssee::net
