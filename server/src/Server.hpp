#pragma once

#include <atomic>
#include <cstdint>

namespace odyssee::server {

/// Serveur de jeu autoritaire. Il possède l'unique état de vérité du monde
/// et le fait avancer à pas de temps fixe (kTickRate ticks/seconde).
class Server {
  public:
    explicit Server(std::uint16_t port);

    /// Boucle principale bloquante ; rend la main quand stop() est appelé.
    void run();
    void stop() { _running = false; }

  private:
    void tick(std::uint64_t tickIndex);

    std::uint16_t _port;
    std::atomic<bool> _running{false};
};

} // namespace odyssee::server
