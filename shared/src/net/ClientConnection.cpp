#include "odyssee/net/ClientConnection.hpp"

#include "odyssee/core/Config.hpp"

#include <utility>

namespace odyssee::net {

namespace {

/// Un Disconnect peut se perdre : on l'envoie plusieurs fois.
constexpr int kDisconnectCopies = 3;

} // namespace

ClientConnection::ClientConnection(std::string playerName) : _playerName(std::move(playerName)) {}

// --- Actions du joueur ------------------------------------------------------------------------

void ClientConnection::connect(Clock::time_point now) {
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
    } else if (std::get_if<Disconnect>(&message) != nullptr) {
        onServerDisconnect();
    }

    if (_state == ConnectionState::Connected) {
        _lastHeard = now; // tout message du serveur prouve qu'il est encore là
    }
}

void ClientConnection::onChallenge(const Challenge& challenge, Clock::time_point now) {
    if (_state != ConnectionState::Requesting) {
        return; // défi en double ou hors de propos
    }
    _token = challenge.token;
    _state = ConnectionState::Answering;
    _stateSince = now;
    _lastSent.reset(); // la réponse part tout de suite
}

void ClientConnection::onAccepted(const Accepted& accepted, Clock::time_point now) {
    if (!isHandshaking()) {
        return; // acceptation en double : déjà connecté
    }
    _playerId = accepted.playerId;
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

void ClientConnection::onServerDisconnect() {
    if (_state == ConnectionState::Connected) {
        _state = ConnectionState::Closed;
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
        return Disconnect{};
    }

    if (isHandshaking() && isDue(kHandshakeResendInterval, now)) {
        _lastSent = now;
        return handshakeMessage();
    }
    if (_state == ConnectionState::Connected && isDue(kKeepAliveInterval, now)) {
        _lastSent = now;
        return KeepAlive{};
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
        return ConnectRequest{_playerName};
    }
    return ChallengeResponse{_token};
}

bool ClientConnection::isDue(Clock::duration interval, Clock::time_point now) const {
    return !_lastSent || now - *_lastSent >= interval;
}

} // namespace odyssee::net
