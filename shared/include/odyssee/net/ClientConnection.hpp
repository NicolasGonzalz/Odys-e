#pragma once

#include "odyssee/net/ControlMessages.hpp"

#include <chrono>
#include <optional>
#include <string>

namespace odyssee::net {

/// Où en est la connexion côté client.
enum class ConnectionState : std::uint8_t {
    Idle,       ///< pas encore lancée
    Requesting, ///< ConnectRequest envoyé, on attend le défi
    Answering,  ///< défi reçu, ChallengeResponse envoyé, on attend l'acceptation
    Connected,  ///< acceptée : playerId() est valide
    Refused,    ///< refusée : voir refuseReason()
    TimedOut,   ///< le serveur ne répond pas (ou plus)
    Closed,     ///< déconnexion volontaire
};

/// Connexion d'un client à un serveur, **sans réseau** : on lui donne les messages reçus,
/// elle dit quoi envoyer. Le transport (socket, plugin Unreal, test) est fait par l'appelant.
///
/// Le temps est passé en paramètre (`now`), comme pour le canal fiable.
class ClientConnection {
  public:
    using Clock = std::chrono::steady_clock;

    /// Le sel de la première tentative est tiré du générateur du système. `inviteCode` : celui
    /// que demande le serveur (T056), vide s'il n'en demande pas.
    explicit ClientConnection(std::string playerName, std::string inviteCode = {});

    /// `salt` : sel de départ imposé, pour des tests reproductibles.
    ClientConnection(std::string playerName, std::uint64_t salt, std::string inviteCode = {});

    /// Lance la poignée de main (avec un nouveau sel : chaque tentative a son jeton de session).
    void connect(Clock::time_point now);

    /// Départ volontaire : les prochains nextOutgoing() renverront quelques Disconnect.
    void disconnect();

    /// Prend en compte un message reçu du serveur. Un message qui ne porte pas le sel ou le jeton
    /// de cette connexion est ignoré : il ne vient pas du serveur (adresse usurpée).
    void onMessage(const ControlMessage& message, Clock::time_point now);

    /// Un paquet de jeu est arrivé du serveur : comme un message, il prouve que le serveur est là.
    void onGamePacketReceived(Clock::time_point now);

    /// Un paquet de jeu vient de partir : pas besoin de KeepAlive tant qu'on en envoie.
    void onGamePacketSent(Clock::time_point now);

    /// Message à envoyer maintenant (demande, réponse au défi, KeepAlive, Disconnect), s'il y en a.
    [[nodiscard]] std::optional<ControlMessage> nextOutgoing(Clock::time_point now);

    /// Vérifie les délais : passe en TimedOut si le serveur ne répond pas.
    void update(Clock::time_point now);

    [[nodiscard]] ConnectionState state() const { return _state; }
    [[nodiscard]] std::optional<PlayerId> playerId() const { return _playerId; }
    /// Empreinte de la carte annoncée par le serveur à l'acceptation (absente avant).
    [[nodiscard]] std::optional<std::uint64_t> serverWorldFingerprint() const {
        return _serverWorld;
    }
    [[nodiscard]] std::optional<RefuseReason> refuseReason() const { return _refuseReason; }

    /// Jeton à mettre dans chaque paquet de jeu (voir Session), une fois connecté.
    [[nodiscard]] std::optional<SessionToken> sessionToken() const;

    /// Sel de la tentative en cours (tests).
    [[nodiscard]] std::uint64_t clientSalt() const { return _salt; }

  private:
    void onChallenge(const Challenge& challenge, Clock::time_point now);
    void onAccepted(const Accepted& accepted, Clock::time_point now);
    void onRefused(const Refused& refused);
    void onServerDisconnect(const Disconnect& disconnect);
    void onKeepAlive(const KeepAlive& keepAlive, Clock::time_point now);

    [[nodiscard]] bool isHandshaking() const;
    [[nodiscard]] std::optional<ControlMessage> handshakeMessage() const;
    [[nodiscard]] bool isDue(Clock::duration interval, Clock::time_point now) const;

    std::string _playerName;
    std::string _inviteCode;
    ConnectionState _state = ConnectionState::Idle;
    std::uint64_t _salt = 0;  ///< sel de la tentative en cours
    std::uint64_t _token = 0; ///< jeton du défi, puis jeton de session
    std::optional<PlayerId> _playerId;
    std::optional<std::uint64_t> _serverWorld;
    std::optional<RefuseReason> _refuseReason;

    Clock::time_point _stateSince{};  ///< début de l'étape en cours (délais de poignée de main)
    Clock::time_point _lastHeard{};   ///< dernier message reçu du serveur
    std::optional<Clock::time_point> _lastSent;
    int _disconnectsToSend = 0;
};

} // namespace odyssee::net
