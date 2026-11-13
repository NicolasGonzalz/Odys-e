#pragma once

#include "odyssee/client/TrafficMeter.hpp"
#include "odyssee/net/Address.hpp"
#include "odyssee/net/ClientConnection.hpp"
#include "odyssee/net/GameEvents.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/Session.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace odyssee::client {

/// Le lien du client avec le serveur : socket UDP, poignée de main (net::ClientConnection), puis
/// session de jeu (net::Session) une fois connecté. Utilisé par le client de debug et, demain,
/// par le plugin Unreal : ils n'ont plus qu'à envoyer des inputs et lire des snapshots.
///
/// Rien ne bloque : poll() lit ce qui est arrivé et envoie ce qui est dû, à appeler à chaque tick.
class ServerLink {
  public:
    using Clock = std::chrono::steady_clock;

    /// Ouvre un socket sur un port libre. std::nullopt si le système le refuse.
    /// `inviteCode` : celui que demande le serveur (T056), vide s'il n'en demande pas.
    [[nodiscard]] static std::optional<ServerLink>
    open(std::string playerName, const net::Address& server, std::string inviteCode = {});

    /// Lance la poignée de main (les messages partent au prochain poll()).
    void connect(Clock::time_point now) { _connection.connect(now); }

    /// Départ volontaire : le serveur est prévenu au prochain poll().
    void disconnect() { _connection.disconnect(); }

    /// Envoie les messages de connexion dus, lit tout ce qui est arrivé, met à jour les délais.
    void poll(Clock::time_point now);

    /// Envoie un paquet de jeu avec ce lot d'inputs (sans effet tant qu'on n'est pas connecté).
    void sendInputs(const net::InputBatch& batch, Clock::time_point now);

    /// Les snapshots reçus depuis le dernier appel, dans l'ordre d'arrivée.
    [[nodiscard]] std::vector<net::Snapshot> takeSnapshots();

    /// Les événements fiables reçus depuis le dernier appel (inventaire…), dans l'ordre d'envoi.
    [[nodiscard]] std::vector<net::GameEvent> takeEvents();

    /// Envoie un événement fiable (demande de déplacement d'objet…) avec le prochain paquet ;
    /// il est renvoyé jusqu'à son acquittement. Faux si pas connecté ou file pleine.
    bool sendEvent(const net::GameEvent& event);

    [[nodiscard]] net::ConnectionState state() const { return _connection.state(); }
    [[nodiscard]] bool connected() const { return state() == net::ConnectionState::Connected; }
    [[nodiscard]] std::optional<net::PlayerId> playerId() const { return _connection.playerId(); }
    /// Empreinte de la carte du serveur (world::fingerprintOf), une fois accepté.
    [[nodiscard]] std::optional<std::uint64_t> serverWorldFingerprint() const {
        return _connection.serverWorldFingerprint();
    }
    [[nodiscard]] std::optional<net::RefuseReason> refuseReason() const {
        return _connection.refuseReason();
    }

    /// Session de jeu (ping, statistiques), une fois connecté.
    [[nodiscard]] const std::optional<net::Session>& session() const { return _session; }

    /// Débit et pertes sur la dernière seconde (HUD réseau, T034).
    [[nodiscard]] TrafficStats traffic(Clock::time_point now) const { return _traffic.stats(now); }

  private:
    ServerLink(std::string playerName, std::string inviteCode, net::UdpSocket socket,
               const net::Address& server);

    void sendControlMessages(Clock::time_point now);
    void sendDatagram(std::span<const std::byte> bytes, Clock::time_point now);
    void openSessionOnceConnected();
    void receiveAll(Clock::time_point now);
    void handle(std::span<const std::byte> datagram, Clock::time_point now);
    void handleGamePacket(const net::IncomingPacket& packet, Clock::time_point now);
    void collectSnapshots();
    void collectEvents();

    net::ClientConnection _connection;
    std::optional<net::Session> _session; ///< créée une fois connecté (il faut le jeton)
    net::UdpSocket _socket;
    net::Address _server;
    std::vector<net::Snapshot> _snapshots;
    std::vector<net::GameEvent> _events;
    TrafficMeter _traffic;
};

} // namespace odyssee::client
