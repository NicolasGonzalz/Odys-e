#pragma once

#include "ClientRegistry.hpp"

#include "odyssee/net/UdpSocket.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

namespace odyssee::server {

/// Relie le socket du serveur au registre des joueurs : lit les paquets, répond aux
/// poignées de main, envoie les KeepAlive et retire les joueurs silencieux.
/// Le serveur appelle poll() à chaque tick.
class ConnectionService {
  public:
    using Clock = std::chrono::steady_clock;

    ConnectionService(net::UdpSocket socket, std::uint64_t seed);

    /// Traite tout ce qui est arrivé depuis le dernier appel, puis fait l'entretien.
    void poll(Clock::time_point now);

    /// Prévient tous les joueurs que le serveur s'arrête.
    void shutdown();

    [[nodiscard]] std::size_t playerCount() const { return _registry.playerCount(); }
    [[nodiscard]] std::uint16_t port() const { return _socket.localPort(); }

  private:
    void receiveAll(Clock::time_point now);
    void handlePacket(const net::Address& from, std::span<const std::byte> bytes,
                      Clock::time_point now);
    void sendKeepAlives(Clock::time_point now);
    void send(const net::Address& to, const net::ControlMessage& message);

    net::UdpSocket _socket;
    ClientRegistry _registry;
};

} // namespace odyssee::server
