#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace odyssee::server {

class ConnectionService;

/// Serveur de jeu autoritaire. Il possède l'unique état de vérité du monde
/// et le fait avancer à pas de temps fixe (kTickRate ticks/seconde).
class Server {
  public:
    using Clock = std::chrono::steady_clock;

    explicit Server(std::uint16_t port);

    /// Ouvre le port UDP puis tourne jusqu'à stop(). Renvoie false si le port n'a pas pu être
    /// ouvert (déjà utilisé, droits insuffisants…).
    [[nodiscard]] bool run();
    void stop() { _running = false; }

  private:
    void loop(ConnectionService& connections);
    void tick(ConnectionService& connections, std::uint64_t tickIndex, Clock::time_point now);

    std::uint16_t _port;
    std::atomic<bool> _running{false};
};

} // namespace odyssee::server
