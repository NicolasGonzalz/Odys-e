#include "ConnectionService.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"
#include "odyssee/net/PacketFraming.hpp"

#include <array>
#include <utility>

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

void ConnectionService::poll(Clock::time_point now) {
    receiveAll(now);
    _registry.dropInactive(now);
    sendKeepAlives(now);
    flushSimulated(now);
}

void ConnectionService::flushSimulated(Clock::time_point now) {
    if (!_simulator) {
        return;
    }
    for (const auto& datagram : _simulator->takeDue(now)) {
        (void)_socket.send(datagram.to, datagram.bytes); // UDP : un échec équivaut à une perte
    }
}

std::optional<ConnectionService::Clock::time_point> ConnectionService::nextSimulatedDue() const {
    return _simulator ? _simulator->nextDue() : std::nullopt;
}

void ConnectionService::shutdown() {
    for (const net::Address& address : _registry.disconnectAll()) {
        sendNow(address, net::Disconnect{});
    }
}

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

    const auto message = net::decodeControl(packet->kind, packet->body);
    if (!message) {
        kLog.debug("paquet invalide ignoré depuis {}", from.toString());
        return;
    }
    if (const auto reply = _registry.handle(from, *message, now)) {
        send(from, *reply, now);
    }
}

void ConnectionService::sendKeepAlives(Clock::time_point now) {
    for (const net::Address& address : _registry.takeKeepAliveTargets(now)) {
        send(address, net::KeepAlive{}, now);
    }
}

void ConnectionService::send(const net::Address& to, const net::ControlMessage& message,
                             Clock::time_point now) {
    if (!_simulator) {
        sendNow(to, message);
        return;
    }
    _simulator->submit(to, net::buildControlPacket(message), now);
}

void ConnectionService::sendNow(const net::Address& to, const net::ControlMessage& message) {
    const auto packet = net::buildControlPacket(message);
    (void)_socket.send(to, packet); // UDP : un échec d'envoi équivaut à une perte
}

} // namespace odyssee::server
