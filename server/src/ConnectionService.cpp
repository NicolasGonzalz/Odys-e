#include "ConnectionService.hpp"

#include "odyssee/core/Log.hpp"

#include <array>
#include <utility>
#include <variant>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"net"};

/// Un client d'une autre version envoie une demande de connexion qu'on ne sait peut-être pas lire :
/// on lui répond quand même « mauvaise version », pour qu'il puisse l'afficher.
bool isForeignConnectRequest(const net::IncomingPacket& packet) {
    return !packet.header.hasCurrentProtocol() && packet.kind == net::PacketKind::ConnectRequest;
}

/// Un Disconnect peut se perdre : à l'arrêt, le serveur l'envoie plusieurs fois (comme le client).
constexpr int kDisconnectCopies = 3;

constexpr std::chrono::seconds kDropReportInterval{10};

/// Le simulateur n'existe que si quelque chose est simulé : sinon, envois directs.
std::optional<net::NetworkSimulator> makeSimulator(const net::NetworkConditions& network,
                                                   std::uint64_t seed) {
    if (network.isPerfect()) {
        return std::nullopt;
    }
    return net::NetworkSimulator(network, seed);
}

} // namespace

ConnectionService::ConnectionService(net::UdpSocket socket, const SipKey& key,
                                     net::NetworkConditions network, Limits limits)
    : _socket(std::move(socket)), _limits(limits), _registry(key, limits.registry),
      _handshakeLimiter(limits.handshakePerIp), _simulator(makeSimulator(network, ~key.k0)) {}

ConnectionService::ConnectionService(net::UdpSocket socket, std::uint64_t seed,
                                     net::NetworkConditions network, Limits limits)
    : ConnectionService(std::move(socket), SipKey::fromSeed(seed), network, limits) {}

// --- Chaque tick ------------------------------------------------------------------------------

void ConnectionService::poll(Clock::time_point now) {
    receiveAll(now);
    _registry.dropInactive(now);
    syncLinks(now);
    sendKeepAlives(now);
    flushSimulated(now);
    reportDropped(now);
}

std::vector<ConnectedPlayer> ConnectionService::connectedPlayers() const {
    std::vector<ConnectedPlayer> players;
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (_links[id]) {
            players.push_back(ConnectedPlayer{id, _links[id]->generation});
        }
    }
    return players;
}

std::vector<net::InputBatch> ConnectionService::takeInputs(net::PlayerId player) {
    std::vector<net::InputBatch> inputs;
    if (player >= kMaxPlayers || !_links[player]) {
        return inputs;
    }
    for (auto& message : _links[player]->session.takeMessages()) {
        if (auto* batch = std::get_if<net::InputBatch>(&message)) {
            inputs.push_back(std::move(*batch)); // un client n'envoie que des inputs
        }
    }
    return inputs;
}

void ConnectionService::queueSnapshot(net::PlayerId player, const net::Snapshot& snapshot) {
    if (player < kMaxPlayers && _links[player]) {
        _links[player]->session.queueUnreliable(snapshot);
    }
}

std::vector<net::GameEvent> ConnectionService::takeEvents(net::PlayerId player) {
    std::vector<net::GameEvent> events;
    if (player >= kMaxPlayers || !_links[player]) {
        return events;
    }
    for (const auto& payload : _links[player]->session.takeReliable()) {
        if (auto event = net::decodeGameEvent(payload)) {
            events.push_back(std::move(*event));
        } else {
            ++_stats.invalid; // donnée réseau hostile : ignorée
        }
    }
    return events;
}

void ConnectionService::queueEvent(net::PlayerId player, const net::GameEvent& event) {
    if (player < kMaxPlayers && _links[player] &&
        !_links[player]->session.queueReliable(net::encodeGameEvent(event))) {
        kLog.warn("joueur {} : trop de messages fiables en attente, événement perdu", player);
    }
}

void ConnectionService::flushGame(Clock::time_point now) {
    for (auto& link : _links) {
        if (!link) {
            continue;
        }
        const auto packet = link->session.buildPacket(now);
        sendBytes(link->address, packet, now);
        _registry.noteSent(link->address, now); // inutile d'envoyer aussi un KeepAlive
    }
    flushSimulated(now);
}

const net::Session* ConnectionService::session(net::PlayerId player) const {
    return player < kMaxPlayers && _links[player] ? &_links[player]->session : nullptr;
}

// --- Réception --------------------------------------------------------------------------------

void ConnectionService::receiveAll(Clock::time_point now) {
    std::array<std::byte, kMaxPacketSize> buffer{};
    // Plafonné : sous un flot de datagrammes, le tick doit quand même avoir lieu.
    for (std::size_t handled = 0; handled < _limits.maxDatagramsPerPoll; ++handled) {
        const auto received = _socket.receive(buffer);
        if (!received) {
            break;
        }
        ++_stats.datagramsHandled;
        handlePacket(received->from, std::span(buffer).first(received->size), now);
    }
}

void ConnectionService::handlePacket(const net::Address& from, std::span<const std::byte> bytes,
                                     Clock::time_point now) {
    const auto packet = net::parsePacket(bytes);
    if (!packet) {
        ++_stats.invalid;
        return; // pas un paquet d'Odyssée
    }

    // Le jeton de session d'abord : sans lui, un paquet n'est jamais traité comme celui d'un
    // joueur, même s'il vient (ou prétend venir) de son adresse — et il ne puise pas non plus
    // dans le débit accordé à ce joueur.
    if (PlayerLink* link = authenticatedLink(from, *packet)) {
        if (link->budget.take(now)) {
            handleSessionPacket(*link, *packet, now);
        } else {
            ++_stats.rateLimited;
        }
        return;
    }
    if (_handshakeLimiter.allow(from.ip, now)) {
        handleUnknown(from, *packet, bytes.size(), now);
    } else {
        ++_stats.rateLimited;
    }
}

/// Paquet de quelqu'un qui n'est pas reconnu comme joueur (débit par IP déjà vérifié).
void ConnectionService::handleUnknown(const net::Address& from, const net::IncomingPacket& packet,
                                      std::size_t requestSize, Clock::time_point now) {
    if (isForeignConnectRequest(packet)) {
        reply(from, net::Refused{net::RefuseReason::WrongVersion}, requestSize, now);
    } else if (packet.header.hasCurrentProtocol()) {
        handleHandshake(from, packet, requestSize, now);
    } else {
        ++_stats.invalid;
    }
}

ConnectionService::PlayerLink* ConnectionService::authenticatedLink(
    const net::Address& from, const net::IncomingPacket& packet) {
    const auto player = _registry.playerAt(from);
    if (!player || !_links[*player] || !packet.header.hasCurrentProtocol()) {
        return nullptr;
    }
    PlayerLink& link = *_links[*player];
    const bool authentic = link.address == from && net::sessionTokenOf(packet) == link.sessionToken;
    return authentic ? &link : nullptr;
}

/// Paquet d'un joueur connecté, jeton vérifié : paquet de jeu, KeepAlive ou Disconnect.
void ConnectionService::handleSessionPacket(PlayerLink& link, const net::IncomingPacket& packet,
                                            Clock::time_point now) {
    if (packet.kind == net::PacketKind::Game) {
        if (link.session.receive(packet, now)) {
            (void)_registry.touch(link.address, now);
        } else {
            ++_stats.invalid; // doublon, trop vieux ou contenu invalide
        }
        return;
    }
    const net::Address from = link.address; // `link` peut disparaître : le joueur part peut-être
    if (const auto message = net::decodeControl(packet.kind, packet.body)) {
        (void)_registry.handle(from, *message, now);
        syncLinks(now);
    }
}

/// Paquet de quelqu'un qui n'est pas (ou pas encore) reconnu comme joueur : seule une poignée
/// de main est possible. Un paquet de jeu ne crée jamais de connexion.
void ConnectionService::handleHandshake(const net::Address& from,
                                        const net::IncomingPacket& packet,
                                        std::size_t requestSize, Clock::time_point now) {
    const bool isHandshake = packet.kind == net::PacketKind::ConnectRequest ||
                             packet.kind == net::PacketKind::ChallengeResponse;
    const auto message =
        isHandshake ? net::decodeControl(packet.kind, packet.body) : std::nullopt;
    if (!message) {
        ++_stats.invalid; // illisible, ou paquet de session sans le bon jeton
        return;
    }
    if (const auto answer = _registry.handle(from, *message, now)) {
        reply(from, *answer, requestSize, now);
    }
    syncLinks(now); // un joueur vient peut-être d'être accepté, ou de se reconnecter
}

/// Une session par joueur connecté : créée à l'acceptation, oubliée au départ. Si une place est
/// reprise (autre client, ou le même qui se reconnecte), la génération change et la session
/// repart de zéro.
void ConnectionService::syncLinks(Clock::time_point now) {
    std::array<std::optional<ClientRegistry::PlayerInfo>, kMaxPlayers> connected{};
    for (auto& info : _registry.players()) {
        const net::PlayerId id = info.id;
        connected[id] = std::move(info);
    }
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        auto& link = _links[id];
        const auto& info = connected[id];
        if (!info) {
            link.reset();
        } else if (!link || link->generation != info->generation) {
            link.emplace(PlayerLink{info->address, info->generation, info->sessionToken,
                                    net::Session{info->sessionToken},
                                    TokenBucket(_limits.perPlayer, now)});
            link->session.acceptOnly(net::GameMessageType::Inputs);
        }
    }
}

// --- Envoi ------------------------------------------------------------------------------------

void ConnectionService::sendKeepAlives(Clock::time_point now) {
    for (const auto& player : _registry.takeKeepAliveTargets(now)) {
        send(player.address, net::KeepAlive{player.sessionToken}, now);
    }
}

void ConnectionService::send(const net::Address& to, const net::ControlMessage& message,
                             Clock::time_point now) {
    sendBytes(to, net::buildControlPacket(message), now);
}

/// Réponse à quelqu'un qui n'a pas encore prouvé son adresse : jamais plus grosse que ce qu'il
/// a envoyé, sinon le serveur servirait d'amplificateur vers une adresse usurpée.
void ConnectionService::reply(const net::Address& to, const net::ControlMessage& message,
                              std::size_t requestSize, Clock::time_point now) {
    const auto packet = net::buildControlPacket(message);
    if (packet.size() <= requestSize) {
        sendBytes(to, packet, now);
    }
}

void ConnectionService::sendBytes(const net::Address& to, std::span<const std::byte> bytes,
                                  Clock::time_point now) {
    if (_simulator) {
        _simulator->submit(to, bytes, now);
        return;
    }
    (void)_socket.send(to, bytes); // UDP : un échec d'envoi équivaut à une perte
}

void ConnectionService::sendNow(const net::Address& to, const net::ControlMessage& message) {
    const auto packet = net::buildControlPacket(message);
    (void)_socket.send(to, packet);
}

void ConnectionService::flushSimulated(Clock::time_point now) {
    if (!_simulator) {
        return;
    }
    for (const auto& datagram : _simulator->takeDue(now)) {
        (void)_socket.send(datagram.to, datagram.bytes);
    }
}

std::optional<ConnectionService::Clock::time_point> ConnectionService::nextSimulatedDue() const {
    return _simulator ? _simulator->nextDue() : std::nullopt;
}

void ConnectionService::shutdown() {
    for (const auto& player : _registry.disconnectAll()) {
        for (int copy = 0; copy < kDisconnectCopies; ++copy) {
            sendNow(player.address, net::Disconnect{player.sessionToken});
        }
    }
    syncLinks(Clock::time_point{});
}

/// Les paquets écartés ne sont pas journalisés un par un (un flot remplirait le journal) : un
/// résumé au plus toutes les dix secondes, s'il y a quelque chose à dire.
void ConnectionService::reportDropped(Clock::time_point now) {
    if (_lastReport && now - *_lastReport < kDropReportInterval) {
        return;
    }
    _lastReport = now;
    _stats.oversized = _socket.oversizedDatagrams();
    const std::uint64_t limited = _stats.rateLimited - _reported.rateLimited;
    const std::uint64_t invalid = _stats.invalid - _reported.invalid;
    const std::uint64_t oversized = _stats.oversized - _reported.oversized;
    if (limited + invalid + oversized > 0) {
        kLog.warn("paquets écartés : {} hors débit, {} invalides ou sans jeton, {} trop gros",
                  limited, invalid, oversized);
    }
    _reported = _stats;
}

} // namespace odyssee::server
