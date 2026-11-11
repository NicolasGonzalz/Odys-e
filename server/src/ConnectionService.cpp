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

/// Le simulateur n'existe que si quelque chose est simulé : sinon, envois directs.
std::optional<net::NetworkSimulator> makeSimulator(const net::NetworkConditions& network,
                                                   std::uint64_t seed) {
    if (network.isPerfect()) {
        return std::nullopt;
    }
    return net::NetworkSimulator(network, seed);
}

} // namespace

ConnectionService::ConnectionService(net::UdpSocket socket, std::uint64_t seed,
                                     net::NetworkConditions network)
    : _socket(std::move(socket)), _registry(seed), _simulator(makeSimulator(network, ~seed)) {}

// --- Chaque tick ------------------------------------------------------------------------------

void ConnectionService::poll(Clock::time_point now) {
    receiveAll(now);
    _registry.dropInactive(now);
    syncLinks();
    sendKeepAlives(now);
    flushSimulated(now);
}

std::vector<net::PlayerId> ConnectionService::connectedPlayers() const {
    std::vector<net::PlayerId> players;
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (_links[id]) {
            players.push_back(id);
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
        send(from, net::Refused{net::RefuseReason::WrongVersion}, now);
        return;
    }
    if (!packet->header.hasCurrentProtocol()) {
        return;
    }

    if (net::isControlPacketKind(packet->kind)) {
        handleControl(from, *packet, now);
    } else {
        handleGame(from, *packet, now);
    }
}

void ConnectionService::handleControl(const net::Address& from, const net::IncomingPacket& packet,
                                      Clock::time_point now) {
    const auto message = net::decodeControl(packet.kind, packet.body);
    if (!message) {
        kLog.debug("paquet invalide ignoré depuis {}", from.toString());
        return;
    }
    if (const auto reply = _registry.handle(from, *message, now)) {
        send(from, *reply, now);
    }
    syncLinks(); // un joueur vient peut-être d'être accepté, ou de partir
}

void ConnectionService::handleGame(const net::Address& from, const net::IncomingPacket& packet,
                                   Clock::time_point now) {
    const auto player = _registry.touch(from, now);
    if (!player || !_links[*player] || _links[*player]->address != from) {
        return; // pas un joueur connecté : un paquet de jeu ne crée jamais de connexion
    }
    if (!_links[*player]->session.receive(packet, now)) {
        kLog.debug("paquet de jeu ignoré depuis {} (doublon ou invalide)", from.toString());
    }
}

/// Une session par joueur connecté : créée à l'acceptation, oubliée au départ. Si une place est
/// reprise par un autre client (autre adresse), la session repart de zéro.
void ConnectionService::syncLinks() {
    std::array<std::optional<net::Address>, kMaxPlayers> connected{};
    for (const auto& info : _registry.players()) {
        connected[info.id] = info.address;
    }
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        auto& link = _links[id];
        if (!connected[id]) {
            link.reset();
        } else if (!link || link->address != *connected[id]) {
            link.emplace(PlayerLink{*connected[id], net::Session{}});
        }
    }
}

// --- Envoi ------------------------------------------------------------------------------------

void ConnectionService::sendKeepAlives(Clock::time_point now) {
    for (const net::Address& address : _registry.takeKeepAliveTargets(now)) {
        send(address, net::KeepAlive{}, now);
    }
}

void ConnectionService::send(const net::Address& to, const net::ControlMessage& message,
                             Clock::time_point now) {
    sendBytes(to, net::buildControlPacket(message), now);
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
    for (const net::Address& address : _registry.disconnectAll()) {
        sendNow(address, net::Disconnect{});
    }
    syncLinks();
}

} // namespace odyssee::server
