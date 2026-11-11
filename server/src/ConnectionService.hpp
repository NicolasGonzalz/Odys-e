#pragma once

#include "ClientRegistry.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/net/NetworkSimulator.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/Session.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace odyssee::server {

/// Relie le socket du serveur aux joueurs : lit les paquets, répond aux poignées de main,
/// retire les joueurs silencieux, et fait circuler les paquets de jeu (une net::Session par
/// joueur connecté). Le serveur appelle poll() au début de chaque tick et flushGame() à la fin.
class ConnectionService {
  public:
    using Clock = std::chrono::steady_clock;

    /// `network` : mauvaises conditions à simuler sur les envois (aucune par défaut).
    ConnectionService(net::UdpSocket socket, std::uint64_t seed,
                      net::NetworkConditions network = {});

    /// Traite tout ce qui est arrivé depuis le dernier appel, puis fait l'entretien.
    void poll(Clock::time_point now);

    /// Les joueurs connectés, par numéro croissant.
    [[nodiscard]] std::vector<net::PlayerId> connectedPlayers() const;

    /// Inputs reçus d'un joueur depuis le dernier appel (vide s'il n'est pas connecté).
    [[nodiscard]] std::vector<net::InputBatch> takeInputs(net::PlayerId player);

    /// Snapshot à envoyer à un joueur dans son prochain paquet.
    void queueSnapshot(net::PlayerId player, const net::Snapshot& snapshot);

    /// Envoie un paquet de jeu à chaque joueur connecté (snapshot, messages fiables dus,
    /// acquittements).
    void flushGame(Clock::time_point now);

    /// Session d'un joueur connecté (statistiques, aller-retour), sinon nullptr.
    [[nodiscard]] const net::Session* session(net::PlayerId player) const;

    /// Envoie les datagrammes retardés par la simulation réseau arrivés à échéance.
    void flushSimulated(Clock::time_point now);

    /// Prochaine échéance de la simulation réseau (le serveur se réveille pour l'honorer).
    [[nodiscard]] std::optional<Clock::time_point> nextSimulatedDue() const;

    /// Prévient tous les joueurs que le serveur s'arrête (sans passer par la simulation :
    /// le serveur ne sera plus là pour envoyer les messages retardés).
    void shutdown();

    [[nodiscard]] std::size_t playerCount() const { return _registry.playerCount(); }
    [[nodiscard]] std::uint16_t port() const { return _socket.localPort(); }

  private:
    /// Ce qui relie le serveur à un joueur connecté, une fois la poignée de main faite.
    struct PlayerLink {
        net::Address address;
        net::Session session;
    };

    void receiveAll(Clock::time_point now);
    void handlePacket(const net::Address& from, std::span<const std::byte> bytes,
                      Clock::time_point now);
    void handleControl(const net::Address& from, const net::IncomingPacket& packet,
                       Clock::time_point now);
    void handleGame(const net::Address& from, const net::IncomingPacket& packet,
                    Clock::time_point now);
    void syncLinks();
    void sendKeepAlives(Clock::time_point now);
    void send(const net::Address& to, const net::ControlMessage& message, Clock::time_point now);
    void sendBytes(const net::Address& to, std::span<const std::byte> bytes,
                   Clock::time_point now);
    void sendNow(const net::Address& to, const net::ControlMessage& message);

    net::UdpSocket _socket;
    ClientRegistry _registry;
    std::optional<net::NetworkSimulator> _simulator; ///< absent : envois directs
    std::array<std::optional<PlayerLink>, kMaxPlayers> _links; ///< indexé par numéro de joueur
};

} // namespace odyssee::server
