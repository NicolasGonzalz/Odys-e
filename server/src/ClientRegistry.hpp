#pragma once

#include "odyssee/net/Address.hpp"
#include "odyssee/net/ControlMessages.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace odyssee::server {

/// Qui est connecté au serveur, qui est en train de se connecter, et quelles places sont libres.
/// Aucune notion de réseau : on lui donne les messages reçus, il dit quoi répondre.
class ClientRegistry {
  public:
    using Clock = std::chrono::steady_clock;

    /// `seed` initialise le générateur des défis (aléatoire en vrai, fixe dans les tests).
    explicit ClientRegistry(std::uint64_t seed);

    /// Traite un message reçu de `from`. Renvoie la réponse à lui envoyer, s'il y en a une.
    [[nodiscard]] std::optional<net::ControlMessage> handle(const net::Address& from,
                                                            const net::ControlMessage& message,
                                                            Clock::time_point now);

    /// Retire les joueurs silencieux depuis trop longtemps et les poignées de main expirées.
    /// Renvoie les numéros des joueurs retirés.
    std::vector<net::PlayerId> dropInactive(Clock::time_point now);

    /// Adresses des joueurs à qui envoyer un KeepAlive maintenant (et note l'envoi).
    [[nodiscard]] std::vector<net::Address> takeKeepAliveTargets(Clock::time_point now);

    /// Retire tous les joueurs (arrêt du serveur) et renvoie leurs adresses, pour les prévenir.
    [[nodiscard]] std::vector<net::Address> disconnectAll();

    /// Un paquet de jeu est arrivé de `from` : si c'est un joueur, il est encore là. Renvoie son
    /// numéro (rien si l'adresse n'est pas celle d'un joueur connecté).
    std::optional<net::PlayerId> touch(const net::Address& from, Clock::time_point now);

    /// Un paquet vient de partir vers `to` : pas besoin de KeepAlive tant qu'on en envoie.
    void noteSent(const net::Address& to, Clock::time_point now);

    struct PlayerInfo {
        net::PlayerId id = 0;
        std::string name;
        net::Address address;
    };

    /// Les joueurs connectés (dans un ordre quelconque).
    [[nodiscard]] std::vector<PlayerInfo> players() const;

    [[nodiscard]] std::size_t playerCount() const { return _players.size(); }
    [[nodiscard]] std::optional<net::PlayerId> playerAt(const net::Address& address) const;

  private:
    struct PendingHandshake {
        std::string playerName;
        std::uint64_t token = 0;
        Clock::time_point startedAt;
    };
    struct Player {
        net::PlayerId id = 0;
        std::string name;
        Clock::time_point lastHeard;
        Clock::time_point lastSent;
    };

    std::optional<net::ControlMessage> onConnectRequest(const net::Address& from,
                                                        const net::ConnectRequest& request,
                                                        Clock::time_point now);
    std::optional<net::ControlMessage> onChallengeResponse(const net::Address& from,
                                                           const net::ChallengeResponse& response,
                                                           Clock::time_point now);
    void onDisconnect(const net::Address& from);

    net::Accepted admit(const net::Address& from, const PendingHandshake& pending,
                        Clock::time_point now);
    [[nodiscard]] std::optional<net::PlayerId> freePlayerId() const;
    [[nodiscard]] bool isFull() const;

    std::mt19937_64 _random;
    std::unordered_map<net::Address, PendingHandshake> _pending;
    std::unordered_map<net::Address, Player> _players;
};

} // namespace odyssee::server
