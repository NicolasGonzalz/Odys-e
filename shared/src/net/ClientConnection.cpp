#include "odyssee/net/ClientConnection.hpp"

#include "odyssee/core/Config.hpp"

#include <random>
#include <utility>

namespace odyssee::net {

namespace {

/// Un Disconnect peut se perdre : on l'envoie plusieurs fois.
constexpr int kDisconnectCopies = 3;

std::uint64_t randomSalt() {
    std::random_device device;
    return (std::uint64_t{device()} << 32) | std::uint64_t{device()};
}

/// Sel suivant (une étape de splitmix64) : deux tentatives n'ont jamais le même.
std::uint64_t nextSalt(std::uint64_t salt) {
    std::uint64_t z = salt + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

} // namespace

ClientConnection::ClientConnection(std::string playerName, std::string inviteCode)
    : ClientConnection(std::move(playerName), randomSalt(), std::move(inviteCode)) {}

ClientConnection::ClientConnection(std::string playerName, std::uint64_t salt,
                                   std::string inviteCode)
    : _playerName(std::move(playerName)), _inviteCode(std::move(inviteCode)), _salt(salt) {}

// --- Actions du joueur ------------------------------------------------------------------------

void ClientConnection::connect(Clock::time_point now) {
    _salt = nextSalt(_salt);
    _token = 0;
    _playerId.reset();
    _serverWorld.reset();
    _refuseReason.reset();
    _state = ConnectionState::Requesting;
    _stateSince = now;
    _lastSent.reset(); // la demande part au prochain nextOutgoing()
}

void ClientConnection::disconnect() {
    if (_state == ConnectionState::Connected || isHandshaking()) {
        _disconnectsToSend = kDisconnectCopies;
    }
    _state = ConnectionState::Closed;
}

// --- Messages reçus ---------------------------------------------------------------------------

void ClientConnection::onMessage(const ControlMessage& message, Clock::time_point now) {
    if (const auto* challenge = std::get_if<Challenge>(&message)) {
        onChallenge(*challenge, now);
    } else if (const auto* accepted = std::get_if<Accepted>(&message)) {
        onAccepted(*accepted, now);
    } else if (const auto* refused = std::get_if<Refused>(&message)) {
        onRefused(*refused);
    } else if (const auto* disconnect = std::get_if<Disconnect>(&message)) {
        onServerDisconnect(*disconnect);
    } else if (const auto* keepAlive = std::get_if<KeepAlive>(&message)) {
        onKeepAlive(*keepAlive, now);
    }
}

void ClientConnection::onChallenge(const Challenge& challenge, Clock::time_point now) {
    if (_state != ConnectionState::Requesting || challenge.clientSalt != _salt) {
        return; // défi en double, hors de propos, ou pas celui de notre demande
    }
    _token = challenge.token;
    _state = ConnectionState::Answering;
    _stateSince = now;
    _lastSent.reset(); // la réponse part tout de suite
}

void ClientConnection::onAccepted(const Accepted& accepted, Clock::time_point now) {
    if (_state == ConnectionState::Connected && accepted.sessionToken == _token) {
        _lastHeard = now; // acceptation en double : le serveur est bien là
        return;
    }
    if (_state != ConnectionState::Answering || accepted.sessionToken != _token) {
        return; // pas l'acceptation de notre réponse au défi
    }
    _playerId = accepted.playerId;
    _serverWorld = accepted.worldFingerprint;
    _state = ConnectionState::Connected;
    _lastHeard = now;
}

void ClientConnection::onRefused(const Refused& refused) {
    if (!isHandshaking()) {
        return;
    }
    _refuseReason = refused.reason;
    _state = ConnectionState::Refused;
}

void ClientConnection::onServerDisconnect(const Disconnect& disconnect) {
    if (_state == ConnectionState::Connected && disconnect.sessionToken == _token) {
        _state = ConnectionState::Closed;
    }
}

void ClientConnection::onKeepAlive(const KeepAlive& keepAlive, Clock::time_point now) {
    if (_state == ConnectionState::Connected && keepAlive.sessionToken == _token) {
        _lastHeard = now; // le serveur est encore là
    }
}

// --- Paquets de jeu ---------------------------------------------------------------------------

void ClientConnection::onGamePacketReceived(Clock::time_point now) {
    if (_state == ConnectionState::Connected) {
        _lastHeard = now;
    }
}

void ClientConnection::onGamePacketSent(Clock::time_point now) {
    if (_state == ConnectionState::Connected) {
        _lastSent = now;
    }
}

// --- Envoi et délais --------------------------------------------------------------------------

std::optional<ControlMessage> ClientConnection::nextOutgoing(Clock::time_point now) {
    if (_disconnectsToSend > 0) {
        --_disconnectsToSend;
        return Disconnect{_token};
    }

    if (isHandshaking() && isDue(kHandshakeResendInterval, now)) {
        _lastSent = now;
        return handshakeMessage();
    }
    if (_state == ConnectionState::Connected && isDue(kKeepAliveInterval, now)) {
        _lastSent = now;
        return KeepAlive{_token};
    }
    return std::nullopt;
}

void ClientConnection::update(Clock::time_point now) {
    const bool handshakeExpired = isHandshaking() && now - _stateSince > kHandshakeTimeout;
    const bool serverSilent =
        _state == ConnectionState::Connected && now - _lastHeard > kConnectionTimeout;

    if (handshakeExpired || serverSilent) {
        _state = ConnectionState::TimedOut;
    }
}

bool ClientConnection::isHandshaking() const {
    return _state == ConnectionState::Requesting || _state == ConnectionState::Answering;
}

std::optional<ControlMessage> ClientConnection::handshakeMessage() const {
    if (_state == ConnectionState::Requesting) {
        return ConnectRequest{_playerName, _salt};
    }
    return ChallengeResponse{_token, _salt, _playerName, _inviteCode};
}

std::optional<SessionToken> ClientConnection::sessionToken() const {
    if (_state != ConnectionState::Connected) {
        return std::nullopt;
    }
    return _token;
}

bool ClientConnection::isDue(Clock::duration interval, Clock::time_point now) const {
    return !_lastSent || now - *_lastSent >= interval;
}

} // namespace odyssee::net
