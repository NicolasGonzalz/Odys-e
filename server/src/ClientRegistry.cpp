#include "ClientRegistry.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/PlayerName.hpp"

#include <iterator>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"server"};

/// Limite des poignées de main simultanées : protège contre un flot de fausses demandes.
constexpr std::size_t kMaxPendingHandshakes = 32;

} // namespace

ClientRegistry::ClientRegistry(std::uint64_t seed) : _random(seed) {}

// --- Aiguillage -------------------------------------------------------------------------------

std::optional<net::ControlMessage> ClientRegistry::handle(const net::Address& from,
                                                          const net::ControlMessage& message,
                                                          Clock::time_point now) {
    if (auto player = _players.find(from); player != _players.end()) {
        player->second.lastHeard = now; // tout message d'un joueur prouve qu'il est là
    }

    if (const auto* request = std::get_if<net::ConnectRequest>(&message)) {
        return onConnectRequest(from, *request, now);
    }
    if (const auto* response = std::get_if<net::ChallengeResponse>(&message)) {
        return onChallengeResponse(from, *response, now);
    }
    if (std::get_if<net::Disconnect>(&message) != nullptr) {
        onDisconnect(from);
    }
    return std::nullopt; // KeepAlive (déjà pris en compte) ou message de serveur reçu à tort
}

// --- Poignée de main --------------------------------------------------------------------------

std::optional<net::ControlMessage> ClientRegistry::onConnectRequest(
    const net::Address& from, const net::ConnectRequest& request, Clock::time_point now) {
    if (const auto id = playerAt(from)) {
        return net::Accepted{*id}; // déjà connecté : notre acceptation s'était perdue
    }
    if (!net::isValidPlayerName(request.playerName)) {
        return net::Refused{net::RefuseReason::InvalidName};
    }
    if (isFull()) {
        return net::Refused{net::RefuseReason::ServerFull};
    }

    // Demande en double : on renvoie le même défi.
    if (const auto pending = _pending.find(from); pending != _pending.end()) {
        return net::Challenge{pending->second.token};
    }
    if (_pending.size() >= kMaxPendingHandshakes) {
        return std::nullopt; // trop de demandes en cours : on ignore (protection)
    }

    const std::uint64_t token = _random();
    _pending.emplace(from, PendingHandshake{request.playerName, token, now});
    return net::Challenge{token};
}

std::optional<net::ControlMessage> ClientRegistry::onChallengeResponse(
    const net::Address& from, const net::ChallengeResponse& response, Clock::time_point now) {
    if (const auto id = playerAt(from)) {
        return net::Accepted{*id};
    }

    const auto pending = _pending.find(from);
    if (pending == _pending.end() || pending->second.token != response.token) {
        return std::nullopt; // pas de défi pour cette adresse, ou mauvaise réponse
    }
    if (isFull()) {
        _pending.erase(pending);
        return net::Refused{net::RefuseReason::ServerFull};
    }

    const auto accepted = admit(from, pending->second, now);
    _pending.erase(pending);
    return accepted;
}

net::Accepted ClientRegistry::admit(const net::Address& from, const PendingHandshake& pending,
                                    Clock::time_point now) {
    const net::PlayerId id = *freePlayerId(); // isFull() a été vérifié avant
    _players.emplace(from, Player{id, pending.playerName, now, now});
    kLog.info("joueur {} « {} » connecté depuis {} ({}/{})", id, pending.playerName,
              from.toString(), _players.size(), kMaxPlayers);
    return net::Accepted{id};
}

void ClientRegistry::onDisconnect(const net::Address& from) {
    _pending.erase(from);
    if (const auto player = _players.find(from); player != _players.end()) {
        kLog.info("joueur {} « {} » s'est déconnecté", player->second.id, player->second.name);
        _players.erase(player);
    }
}

// --- Entretien --------------------------------------------------------------------------------

std::vector<net::PlayerId> ClientRegistry::dropInactive(Clock::time_point now) {
    std::erase_if(_pending, [now](const auto& entry) {
        return now - entry.second.startedAt > kHandshakeTimeout;
    });

    std::vector<net::PlayerId> dropped;
    std::erase_if(_players, [&](const auto& entry) {
        const Player& player = entry.second;
        if (now - player.lastHeard <= kConnectionTimeout) {
            return false;
        }
        kLog.info("joueur {} « {} » ne répond plus : déconnecté", player.id, player.name);
        dropped.push_back(player.id);
        return true;
    });
    return dropped;
}

std::vector<net::Address> ClientRegistry::takeKeepAliveTargets(Clock::time_point now) {
    std::vector<net::Address> targets;
    for (auto& [address, player] : _players) {
        if (now - player.lastSent >= kKeepAliveInterval) {
            player.lastSent = now;
            targets.push_back(address);
        }
    }
    return targets;
}

std::vector<net::Address> ClientRegistry::disconnectAll() {
    std::vector<net::Address> addresses;
    addresses.reserve(_players.size());
    for (const auto& entry : _players) {
        addresses.push_back(entry.first);
    }
    _players.clear();
    _pending.clear();
    return addresses;
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
        list.push_back(PlayerInfo{player.id, player.name, address});
    }
    return list;
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

bool ClientRegistry::isFull() const { return _players.size() >= kMaxPlayers; }

} // namespace odyssee::server
