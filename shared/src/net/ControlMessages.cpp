#include "odyssee/net/ControlMessages.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/PacketFraming.hpp"

namespace odyssee::net {

namespace {

/// On accepte de lire un pseudo plus long que permis, pour pouvoir répondre « pseudo invalide »
/// au lieu d'ignorer la demande. La règle elle-même est dans PlayerName.
constexpr std::size_t kMaxNameOnWire = 64;
/// Idem pour le code d'invitation (tirets et espaces compris) : « code faux » plutôt que silence.
constexpr std::size_t kMaxInviteCodeOnWire = 64;

// --- Écriture : une fonction par message ------------------------------------------------------

void writeBody(ByteWriter& w, const ConnectRequest& m) {
    w.writeU64(m.clientSalt);
    w.writeString(m.playerName);
    while (w.size() < kConnectRequestBodySize) {
        w.writeU8(0); // rembourrage : voir kConnectRequestBodySize
    }
}
void writeBody(ByteWriter& w, const Challenge& m) {
    w.writeU64(m.clientSalt);
    w.writeU64(m.token);
}
void writeBody(ByteWriter& w, const ChallengeResponse& m) {
    w.writeU64(m.clientSalt);
    w.writeU64(m.token);
    w.writeString(m.playerName);
    w.writeString(m.inviteCode);
}
void writeBody(ByteWriter& w, const Accepted& m) {
    w.writeU8(m.playerId);
    w.writeU64(m.sessionToken);
    w.writeU64(m.worldFingerprint);
}
void writeBody(ByteWriter& w, const Refused& m) { w.writeU8(static_cast<std::uint8_t>(m.reason)); }
void writeBody(ByteWriter& w, const Disconnect& m) { w.writeU64(m.sessionToken); }
void writeBody(ByteWriter& w, const KeepAlive& m) { w.writeU64(m.sessionToken); }

// --- Lecture : une fonction par message -------------------------------------------------------

std::optional<ControlMessage> readConnectRequest(ByteReader& r) {
    if (r.remaining() < kConnectRequestBodySize) {
        return std::nullopt; // pas rembourrée : refusée (anti-amplification)
    }
    const auto salt = r.readU64();
    auto name = r.readString(kMaxNameOnWire);
    if (!salt || !name) {
        return std::nullopt;
    }
    (void)r.readBytes(r.remaining()); // le rembourrage, quel qu'il soit
    return ConnectRequest{std::move(*name), *salt};
}

std::optional<ControlMessage> readChallenge(ByteReader& r) {
    const auto salt = r.readU64();
    const auto token = r.readU64();
    if (!salt || !token) {
        return std::nullopt;
    }
    return Challenge{*token, *salt};
}

std::optional<ControlMessage> readChallengeResponse(ByteReader& r) {
    const auto salt = r.readU64();
    const auto token = r.readU64();
    auto name = r.readString(kMaxNameOnWire);
    auto code = r.readString(kMaxInviteCodeOnWire);
    if (!salt || !token || !name || !code) {
        return std::nullopt;
    }
    return ChallengeResponse{*token, *salt, std::move(*name), std::move(*code)};
}

std::optional<ControlMessage> readAccepted(ByteReader& r) {
    const auto id = r.readU8();
    const auto token = r.readU64();
    const auto world = r.readU64();
    if (!id || *id >= kMaxPlayers || !token || !world) {
        return std::nullopt;
    }
    return Accepted{*id, *token, *world};
}

std::optional<ControlMessage> readRefused(ByteReader& r) {
    const auto reason = r.readU8();
    const bool known = reason && *reason >= 1 && *reason <= 5;
    if (!known) {
        return std::nullopt;
    }
    return Refused{static_cast<RefuseReason>(*reason)};
}

template <typename Message> std::optional<ControlMessage> readSessionMessage(ByteReader& r) {
    const auto token = r.readU64();
    if (!token) {
        return std::nullopt;
    }
    return Message{*token};
}

std::optional<ControlMessage> readMessage(PacketKind kind, ByteReader& r) {
    switch (kind) {
    case PacketKind::ConnectRequest:
        return readConnectRequest(r);
    case PacketKind::Challenge:
        return readChallenge(r);
    case PacketKind::ChallengeResponse:
        return readChallengeResponse(r);
    case PacketKind::Accepted:
        return readAccepted(r);
    case PacketKind::Refused:
        return readRefused(r);
    case PacketKind::Disconnect:
        return readSessionMessage<Disconnect>(r);
    case PacketKind::KeepAlive:
        return readSessionMessage<KeepAlive>(r);
    case PacketKind::Game:
        return std::nullopt; // paquet de jeu : voir GamePacket
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
    case RefuseReason::TooManyFromAddress:
        return "trop de joueurs connectés depuis cette adresse";
    case RefuseReason::WrongInviteCode:
        return "code d'invitation absent ou faux";
    }
    return "raison inconnue";
}

} // namespace odyssee::net
