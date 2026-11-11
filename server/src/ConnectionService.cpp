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
                                     net::NetworkConditions network)
    : _socket(std::move(socket)), _registry(key), _simulator(makeSimulator(network, ~key.k0)) {}

ConnectionService::ConnectionService(net::UdpSocket socket, std::uint64_t seed,
                                     net::NetworkConditions network)
    : ConnectionService(std::move(socket), SipKey::fromSeed(seed), network) {}

// --- Chaque tick ------------------------------------------------------------------------------

void ConnectionService::poll(Clock::time_point now) {
    receiveAll(now);
    _registry.dropInactive(now);
    syncLinks();
    sendKeepAlives(now);
    flushSimulated(now);
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
    while (const auto received = _socket.receive(buffer)) {
        handlePacket(received->from, std::span(buffer).first(received->size), now);
    }
}

void ConnectionService::handlePacket(const net::Address& from, std::span<const std::byte> bytes,
                                     Clock::time_point now) {
    const auto packet = net::parsePacket(bytes);
    if (!packet) {
        return; // pas un paquet d'Odyssée
    }

    if (isForeignConnectRequest(*packet)) {
        reply(from, net::Refused{net::RefuseReason::WrongVersion}, bytes.size(), now);
        return;
    }
    if (!packet->header.hasCurrentProtocol()) {
        return;
    }

    // Le jeton de session d'abord : sans lui, un paquet n'est jamais traité comme celui d'un
    // joueur, même s'il vient (ou prétend venir) de son adresse.
    if (PlayerLink* link = authenticatedLink(from, *packet)) {
        handleSessionPacket(*link, *packet, now);
    } else {
        handleHandshake(from, *packet, bytes.size(), now);
    }
}

ConnectionService::PlayerLink* ConnectionService::authenticatedLink(
    const net::Address& from, const net::IncomingPacket& packet) {
    const auto player = _registry.playerAt(from);
    if (!player || !_links[*player]) {
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
            kLog.debug("paquet de jeu ignoré depuis {} (doublon ou invalide)",
                       link.address.toString());
        }
        return;
    }
    const net::Address from = link.address; // `link` peut disparaître : le joueur part peut-être
    if (const auto message = net::decodeControl(packet.kind, packet.body)) {
        (void)_registry.handle(from, *message, now);
        syncLinks();
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
        kLog.debug("paquet ignoré depuis {} (invalide ou sans jeton de session)",
                   from.toString());
        return;
    }
    if (const auto answer = _registry.handle(from, *message, now)) {
        reply(from, *answer, requestSize, now);
    }
    syncLinks(); // un joueur vient peut-être d'être accepté, ou de se reconnecter
}

/// Une session par joueur connecté : créée à l'acceptation, oubliée au départ. Si une place est
/// reprise (autre client, ou le même qui se reconnecte), la génération change et la session
/// repart de zéro.
void ConnectionService::syncLinks() {
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
                                    net::Session{info->sessionToken}});
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
    syncLinks();
}

} // namespace odyssee::server
