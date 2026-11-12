#pragma once

#include "ChallengeTokens.hpp"
#include "ConnectedPlayer.hpp"
#include "SipHash.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/Address.hpp"
#include "odyssee/net/ControlMessages.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace odyssee::server {

/// Réglages de ClientRegistry. Hors de la classe : GCC et Clang refusent qu'une structure
/// imbriquée avec des valeurs par défaut serve d'argument par défaut (`= {}`) dans sa classe.
struct RegistryLimits {
    /// Joueurs connectés depuis une même adresse IP. Par défaut, pas plus strict que le
    /// nombre de places : quatre amis derrière la même box partagent une IP.
    std::size_t maxPlayersPerIp = kMaxPlayers;
    /// Code d'invitation exigé (T056), normalisé (net::normalizeInviteCode). Vide : serveur
    /// ouvert à tous (en local, ou derrière un VPN).
    std::string inviteCode;
};

/// Qui est connecté au serveur, et quelles places sont libres. Aucune notion de réseau : on lui
/// donne les messages reçus, il dit quoi répondre.
///
/// La poignée de main est **sans état** (voir ChallengeTokens) : rien n'est retenu avant qu'un
/// client ait prouvé, en renvoyant le défi, qu'il reçoit bien les paquets à son adresse.
class ClientRegistry {
  public:
    using Clock = std::chrono::steady_clock;

    using Limits = RegistryLimits;

    /// `seed` : secret des défis dérivé d'une graine, pour des tests reproductibles.
    explicit ClientRegistry(std::uint64_t seed, Limits limits = {});

    /// `key` : secret des défis (SipKey::random() en vrai).
    explicit ClientRegistry(const SipKey& key, Limits limits = {});

    /// Traite un message reçu de `from`. Renvoie la réponse à lui envoyer, s'il y en a une.
    /// Un Disconnect ou un KeepAlive sans le jeton de session du joueur est ignoré.
    [[nodiscard]] std::optional<net::ControlMessage> handle(const net::Address& from,
                                                            const net::ControlMessage& message,
                                                            Clock::time_point now);

    /// Retire les joueurs silencieux depuis trop longtemps. Renvoie leurs numéros.
    std::vector<net::PlayerId> dropInactive(Clock::time_point now);

    struct PlayerInfo {
        net::PlayerId id = 0;
        std::string name;
        net::Address address;
        ConnectionGeneration generation = 0;
        net::SessionToken sessionToken = 0;
    };

    /// Joueurs à qui envoyer un KeepAlive maintenant (et note l'envoi).
    [[nodiscard]] std::vector<PlayerInfo> takeKeepAliveTargets(Clock::time_point now);

    /// Retire tous les joueurs (arrêt du serveur) et les renvoie, pour les prévenir.
    [[nodiscard]] std::vector<PlayerInfo> disconnectAll();

    /// Un paquet de jeu **authentifié** (bon jeton de session) est arrivé de `from` : le joueur
    /// est encore là. Renvoie son numéro (rien si l'adresse n'est pas celle d'un joueur).
    std::optional<net::PlayerId> touch(const net::Address& from, Clock::time_point now);

    /// Un paquet vient de partir vers `to` : pas besoin de KeepAlive tant qu'on en envoie.
    void noteSent(const net::Address& to, Clock::time_point now);

    /// Les joueurs connectés (dans un ordre quelconque).
    [[nodiscard]] std::vector<PlayerInfo> players() const;

    [[nodiscard]] std::size_t playerCount() const { return _players.size(); }
    [[nodiscard]] std::optional<net::PlayerId> playerAt(const net::Address& address) const;

  private:
    struct Player {
        net::PlayerId id = 0;
        std::string name;
        ConnectionGeneration generation = 0;
        net::SessionToken sessionToken = 0;
        Clock::time_point lastHeard;
        Clock::time_point lastSent;
    };
    using Players = std::unordered_map<net::Address, Player>;

    std::optional<net::ControlMessage> onConnectRequest(const net::Address& from,
                                                        const net::ConnectRequest& request,
                                                        Clock::time_point now);
    std::optional<net::ControlMessage> onChallengeResponse(const net::Address& from,
                                                           const net::ChallengeResponse& response,
                                                           Clock::time_point now);
    void onDisconnect(const net::Address& from, const net::Disconnect& disconnect);
    void onKeepAlive(const net::Address& from, const net::KeepAlive& keepAlive,
                     Clock::time_point now);

    /// Raison de refuser un nouveau joueur à cette adresse, s'il y en a une. Le joueur déjà
    /// connecté à cette adresse ne compte pas : s'il revient, il libère sa place.
    [[nodiscard]] bool hasInvitation(const net::ChallengeResponse& response) const;
    [[nodiscard]] std::optional<net::RefuseReason> refusalFor(const net::Address& from) const;
    net::Accepted admit(const net::Address& from, const net::ChallengeResponse& response,
                        Clock::time_point now);
    void remove(Players::iterator player);
    void retire(net::SessionToken token);
    [[nodiscard]] bool isRetired(net::SessionToken token) const;
    [[nodiscard]] std::optional<net::PlayerId> freePlayerId() const;
    [[nodiscard]] static PlayerInfo infoOf(const net::Address& address, const Player& player);

    ChallengeTokens _tokens;
    Limits _limits;
    Players _players;
    ConnectionGeneration _nextGeneration = 1;

    /// Jetons des dernières sessions terminées : une vieille réponse au défi, rejouée ou
    /// arrivée en retard, ne doit pas recréer un joueur fantôme tant que son jeton est valable.
    std::array<std::optional<net::SessionToken>, 64> _retired{};
    std::size_t _nextRetired = 0;
};

} // namespace odyssee::server
