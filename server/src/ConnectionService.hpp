#pragma once

#include "ClientRegistry.hpp"
#include "IpRateLimiter.hpp"
#include "TokenBucket.hpp"

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

/// Ce que le serveur accepte de traiter : un expéditeur trop bavard est écarté sans coûter
/// plus que la lecture de son datagramme. (Hors de la classe : voir RegistryLimits.)
struct ConnectionLimits {
    /// Datagrammes traités par appel à poll() : un flot ne peut pas retenir le tick. Le
    /// reste attend dans le tampon du système (qui jette quand il est plein).
    std::size_t maxDatagramsPerPoll = 1024;
    /// Paquets de poignée de main (ou sans jeton de session valide), par adresse IP.
    TokenBucket::Rate handshakePerIp{32, 16};
    /// Paquets d'un joueur connecté (jeton vérifié) : large pour un client à 60 paquets/s.
    TokenBucket::Rate perPlayer{120, 120};
    RegistryLimits registry{};
};

/// Relie le socket du serveur aux joueurs : lit les paquets, répond aux poignées de main,
/// retire les joueurs silencieux, et fait circuler les paquets de jeu (une net::Session par
/// joueur connecté). Le serveur appelle poll() au début de chaque tick et flushGame() à la fin.
class ConnectionService {
  public:
    using Clock = std::chrono::steady_clock;

    using Limits = ConnectionLimits;

    /// Ce qui a été reçu et écarté depuis le démarrage (tests, journal).
    struct Stats {
        std::uint64_t datagramsHandled = 0;
        std::uint64_t rateLimited = 0; ///< écartés : seau à jetons vide
        std::uint64_t invalid = 0;     ///< écartés : illisibles, hors de propos, mauvais jeton
        std::uint64_t oversized = 0;   ///< écartés par le socket : trop gros (au dernier résumé)
    };

    /// `key` : secret des défis de connexion (SipKey::random() en vrai).
    /// `network` : mauvaises conditions à simuler sur les envois (aucune par défaut).
    ConnectionService(net::UdpSocket socket, const SipKey& key,
                      net::NetworkConditions network = {}, Limits limits = {});

    /// Secret et tirages dérivés de `seed` : pour des tests reproductibles.
    ConnectionService(net::UdpSocket socket, std::uint64_t seed,
                      net::NetworkConditions network = {}, Limits limits = {});

    /// Traite tout ce qui est arrivé depuis le dernier appel, puis fait l'entretien.
    void poll(Clock::time_point now);

    /// Les joueurs connectés, par numéro croissant, avec la génération de leur connexion.
    [[nodiscard]] std::vector<ConnectedPlayer> connectedPlayers() const;

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

    [[nodiscard]] const Stats& stats() const { return _stats; }
    [[nodiscard]] std::size_t playerCount() const { return _registry.playerCount(); }
    [[nodiscard]] std::uint16_t port() const { return _socket.localPort(); }

  private:
    /// Ce qui relie le serveur à un joueur connecté, une fois la poignée de main faite.
    struct PlayerLink {
        net::Address address;
        ConnectionGeneration generation = 0;
        net::SessionToken sessionToken = 0;
        net::Session session;
        TokenBucket budget; ///< débit accordé à ce joueur
    };

    void receiveAll(Clock::time_point now);
    void handlePacket(const net::Address& from, std::span<const std::byte> bytes,
                      Clock::time_point now);
    [[nodiscard]] PlayerLink* authenticatedLink(const net::Address& from,
                                                const net::IncomingPacket& packet);
    void handleSessionPacket(PlayerLink& link, const net::IncomingPacket& packet,
                             Clock::time_point now);
    void handleHandshake(const net::Address& from, const net::IncomingPacket& packet,
                         std::size_t requestSize, Clock::time_point now);
    void reply(const net::Address& to, const net::ControlMessage& message,
               std::size_t requestSize, Clock::time_point now);
    void handleUnknown(const net::Address& from, const net::IncomingPacket& packet,
                       std::size_t requestSize, Clock::time_point now);
    void syncLinks(Clock::time_point now);
    void reportDropped(Clock::time_point now);
    void sendKeepAlives(Clock::time_point now);
    void send(const net::Address& to, const net::ControlMessage& message, Clock::time_point now);
    void sendBytes(const net::Address& to, std::span<const std::byte> bytes,
                   Clock::time_point now);
    void sendNow(const net::Address& to, const net::ControlMessage& message);

    net::UdpSocket _socket;
    Limits _limits;
    ClientRegistry _registry;
    IpRateLimiter _handshakeLimiter;
    Stats _stats;
    Stats _reported; ///< valeurs de _stats au dernier résumé journalisé
    std::optional<Clock::time_point> _lastReport;
    std::optional<net::NetworkSimulator> _simulator; ///< absent : envois directs
    std::array<std::optional<PlayerLink>, kMaxPlayers> _links; ///< indexé par numéro de joueur
};

} // namespace odyssee::server
