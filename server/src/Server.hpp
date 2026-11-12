#pragma once

#include "GameWorld.hpp"

#include "odyssee/net/NetworkSimulator.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

namespace odyssee::server {

class ConnectionService;

/// Serveur de jeu autoritaire. Il possède l'unique état de vérité du monde
/// et le fait avancer à pas de temps fixe (kTickRate ticks/seconde).
class Server {
  public:
    using Clock = std::chrono::steady_clock;

    /// `network` : mauvaises conditions réseau à simuler sur les envois (aucune par défaut).
    /// `world` : la carte et son point d'apparition (sol plat provisoire par défaut).
    /// `inviteCode` : code exigé des joueurs (T056) ; vide : serveur ouvert à tous.
    explicit Server(std::uint16_t port, net::NetworkConditions network = {}, GameWorld world = {},
                    std::string inviteCode = {});

    /// Ouvre le port UDP puis tourne jusqu'à stop(). Renvoie false si le port n'a pas pu être
    /// ouvert (déjà utilisé, droits insuffisants…).
    [[nodiscard]] bool run();
    void stop() { _running = false; }

  private:
    void loop(ConnectionService& connections);
    void tick(ConnectionService& connections, std::uint64_t tickIndex, Clock::time_point now);
    static void waitUntil(ConnectionService& connections, Clock::time_point nextTick);

    std::uint16_t _port;
    net::NetworkConditions _network;
    GameWorld _world;
    std::string _inviteCode; ///< normalisé ; vide : pas de code exigé
    std::atomic<bool> _running{false};
};

} // namespace odyssee::server
