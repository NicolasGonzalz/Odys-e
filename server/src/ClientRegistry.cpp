#include "ClientRegistry.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/InviteCode.hpp"
#include "odyssee/net/PlayerName.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};

} // namespace

ClientRegistry::ClientRegistry(std::uint64_t seed, Limits limits)
    : ClientRegistry(SipKey::fromSeed(seed), limits) {}

ClientRegistry::ClientRegistry(const SipKey& key, Limits limits) : _tokens(key), _limits(limits) {}

// --- Aiguillage -------------------------------------------------------------------------------

std::optional<net::ControlMessage> ClientRegistry::handle(const net::Address& from,
                                                          const net::ControlMessage& message,
                                                          Clock::time_point now) {
    if (const auto* request = std::get_if<net::ConnectRequest>(&message)) {
        return onConnectRequest(from, *request, now);
    }
    if (const auto* response = std::get_if<net::ChallengeResponse>(&message)) {
        return onChallengeResponse(from, *response, now);
    }
    if (const auto* disconnect = std::get_if<net::Disconnect>(&message)) {
        onDisconnect(from, *disconnect);
    } else if (const auto* keepAlive = std::get_if<net::KeepAlive>(&message)) {
        onKeepAlive(from, *keepAlive, now);
    }
    return std::nullopt; // rien à répondre, ou message de serveur reçu à tort
}

// --- Poignée de main --------------------------------------------------------------------------

/// Rien n'est retenu ici : le défi est recalculable (ChallengeTokens). Une demande, même venue de
/// l'adresse d'un joueur connecté, n'a donc aucun effet tant que le défi n'a pas été renvoyé :
/// usurper l'adresse d'un joueur ne permet pas de le déconnecter.
std::optional<net::ControlMessage> ClientRegistry::onConnectRequest(
    const net::Address& from, const net::ConnectRequest& request, Clock::time_point now) {
    if (!net::isValidPlayerName(request.playerName)) {
        return net::Refused{net::RefuseReason::InvalidName};
    }
    if (const auto reason = refusalFor(from)) {
        return net::Refused{*reason};
    }
    const ChallengeTokens::Claim claim{from, request.clientSalt, request.playerName};
    return net::Challenge{_tokens.issue(claim, now), request.clientSalt};
}

std::optional<net::ControlMessage> ClientRegistry::onChallengeResponse(
    const net::Address& from, const net::ChallengeResponse& response, Clock::time_point now) {
    const ChallengeTokens::Claim claim{from, response.clientSalt, response.playerName};
    if (!net::isValidPlayerName(response.playerName) ||
        !_tokens.verify(claim, response.token, now)) {
        return std::nullopt; // jeton faux, périmé, ou émis pour une autre adresse
    }

    const auto current = _players.find(from);
    if (current != _players.end() && current->second.sessionToken == response.token) {
        // Notre acceptation s'était perdue : le client renvoie la même réponse.
        return net::Accepted{current->second.id, response.token};
    }
    if (isRetired(response.token)) {
        return std::nullopt; // réponse d'une session déjà terminée (rejouée ou en retard)
    }
    if (!hasInvitation(response)) {
        kLog.warn("{} « {} » refusé : code d'invitation absent ou faux", from.toString(),
                  response.playerName);
        return net::Refused{net::RefuseReason::WrongInviteCode};
    }
    if (const auto reason = refusalFor(from)) {
        return net::Refused{*reason};
    }
    if (current != _players.end()) {
        // Nouvelle poignée de main réussie depuis la même adresse : le client a redémarré.
        // L'ancienne connexion est oubliée, la nouvelle repart de zéro (nouvelle génération).
        kLog.info("joueur {} « {} » se reconnecte", current->second.id, current->second.name);
        remove(current);
    }
    return admit(from, response, now);
}

net::Accepted ClientRegistry::admit(const net::Address& from,
                                    const net::ChallengeResponse& response,
                                    Clock::time_point now) {
    const net::PlayerId id = *freePlayerId(); // refusalFor() a vérifié qu'il reste une place
    _players.emplace(from, Player{id, response.playerName, _nextGeneration++, response.token,
                                  now, now});
    kLog.info("joueur {} « {} » connecté depuis {} ({}/{})", id, response.playerName,
              from.toString(), _players.size(), kMaxPlayers);
    return net::Accepted{id, response.token};
}

bool ClientRegistry::hasInvitation(const net::ChallengeResponse& response) const {
    return _limits.inviteCode.empty() ||
           net::inviteCodeMatches(_limits.inviteCode, response.inviteCode);
}

std::optional<net::RefuseReason> ClientRegistry::refusalFor(const net::Address& from) const {
    std::size_t others = 0;
    std::size_t othersAtSameIp = 0;
    for (const auto& entry : _players) {
        if (entry.first == from) {
            continue;
        }
        ++others;
        othersAtSameIp += entry.first.ip == from.ip ? 1U : 0U;
    }
    if (others >= kMaxPlayers) {
        return net::RefuseReason::ServerFull;
    }
    if (othersAtSameIp >= _limits.maxPlayersPerIp) {
        return net::RefuseReason::TooManyFromAddress;
    }
    return std::nullopt;
}

// --- Messages d'un joueur connecté --------------------------------------------------------------

void ClientRegistry::onDisconnect(const net::Address& from, const net::Disconnect& disconnect) {
    const auto player = _players.find(from);
    if (player == _players.end() || player->second.sessionToken != disconnect.sessionToken) {
        return; // pas un joueur, ou pas son jeton : adresse usurpée
    }
    kLog.info("joueur {} « {} » s'est déconnecté", player->second.id, player->second.name);
    remove(player);
}

void ClientRegistry::onKeepAlive(const net::Address& from, const net::KeepAlive& keepAlive,
                                 Clock::time_point now) {
    const auto player = _players.find(from);
    if (player != _players.end() && player->second.sessionToken == keepAlive.sessionToken) {
        player->second.lastHeard = now;
    }
}

// --- Entretien --------------------------------------------------------------------------------

std::vector<net::PlayerId> ClientRegistry::dropInactive(Clock::time_point now) {
    std::vector<net::PlayerId> dropped;
    for (auto player = _players.begin(); player != _players.end();) {
        if (now - player->second.lastHeard <= kConnectionTimeout) {
            ++player;
            continue;
        }
        kLog.info("joueur {} « {} » ne répond plus : déconnecté", player->second.id,
                  player->second.name);
        dropped.push_back(player->second.id);
        retire(player->second.sessionToken);
        player = _players.erase(player);
    }
    return dropped;
}

std::vector<ClientRegistry::PlayerInfo> ClientRegistry::takeKeepAliveTargets(
    Clock::time_point now) {
    std::vector<PlayerInfo> targets;
    for (auto& [address, player] : _players) {
        if (now - player.lastSent >= kKeepAliveInterval) {
            player.lastSent = now;
            targets.push_back(infoOf(address, player));
        }
    }
    return targets;
}

std::vector<ClientRegistry::PlayerInfo> ClientRegistry::disconnectAll() {
    std::vector<PlayerInfo> everyone = players();
    for (const PlayerInfo& info : everyone) {
        retire(info.sessionToken);
    }
    _players.clear();
    return everyone;
}

// --- Trafic de jeu ----------------------------------------------------------------------------

std::optional<net::PlayerId> ClientRegistry::touch(const net::Address& from,
                                                   Clock::time_point now) {
    const auto player = _players.find(from);
    if (player == _players.end()) {
        return std::nullopt;
    }
    player->second.lastHeard = now;
    return player->second.id;
}

void ClientRegistry::noteSent(const net::Address& to, Clock::time_point now) {
    if (auto player = _players.find(to); player != _players.end()) {
        player->second.lastSent = now;
    }
}

std::vector<ClientRegistry::PlayerInfo> ClientRegistry::players() const {
    std::vector<PlayerInfo> list;
    list.reserve(_players.size());
    for (const auto& [address, player] : _players) {
        list.push_back(infoOf(address, player));
    }
    return list;
}

ClientRegistry::PlayerInfo ClientRegistry::infoOf(const net::Address& address,
                                                  const Player& player) {
    return PlayerInfo{player.id, player.name, address, player.generation, player.sessionToken};
}

// --- Places -----------------------------------------------------------------------------------

std::optional<net::PlayerId> ClientRegistry::playerAt(const net::Address& address) const {
    const auto player = _players.find(address);
    if (player == _players.end()) {
        return std::nullopt;
    }
    return player->second.id;
}

std::optional<net::PlayerId> ClientRegistry::freePlayerId() const {
    std::array<bool, kMaxPlayers> taken{};
    for (const auto& entry : _players) {
        taken[entry.second.id] = true;
    }
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (!taken[id]) {
            return id;
        }
    }
    return std::nullopt;
}

// --- Sessions terminées -----------------------------------------------------------------------

void ClientRegistry::remove(Players::iterator player) {
    retire(player->second.sessionToken);
    _players.erase(player);
}

void ClientRegistry::retire(net::SessionToken token) {
    _retired[_nextRetired] = token;
    _nextRetired = (_nextRetired + 1) % _retired.size();
}

bool ClientRegistry::isRetired(net::SessionToken token) const {
    return std::find(_retired.begin(), _retired.end(), std::optional{token}) != _retired.end();
}

} // namespace odyssee::server
