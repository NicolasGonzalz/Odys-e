#pragma once

#include "ClientRegistry.hpp"

#include "odyssee/net/NetworkSimulator.hpp"
#include "odyssee/net/UdpSocket.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace odyssee::server {

/// Relie le socket du serveur au registre des joueurs : lit les paquets, répond aux
/// poignées de main, envoie les KeepAlive et retire les joueurs silencieux.
/// Le serveur appelle poll() à chaque tick.
class ConnectionService {
  public:
    using Clock = std::chrono::steady_clock;

    /// `network` : mauvaises conditions à simuler sur les envois (aucune par défaut).
    ConnectionService(net::UdpSocket socket, std::uint64_t seed,
                      net::NetworkConditions network = {});

    /// Traite tout ce qui est arrivé depuis le dernier appel, puis fait l'entretien.
    void poll(Clock::time_point now);

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
    void receiveAll(Clock::time_point now);
    void handlePacket(const net::Address& from, std::span<const std::byte> bytes,
                      Clock::time_point now);
    void sendKeepAlives(Clock::time_point now);
    void send(const net::Address& to, const net::ControlMessage& message, Clock::time_point now);
    void sendNow(const net::Address& to, const net::ControlMessage& message);

    net::UdpSocket _socket;
    ClientRegistry _registry;
    std::optional<net::NetworkSimulator> _simulator; ///< absent : envois directs
};

} // namespace odyssee::server
