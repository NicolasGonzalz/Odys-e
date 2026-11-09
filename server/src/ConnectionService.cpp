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

} // namespace

ConnectionService::ConnectionService(net::UdpSocket socket, std::uint64_t seed)
    : _socket(std::move(socket)), _registry(seed) {}

void ConnectionService::poll(Clock::time_point now) {
    receiveAll(now);
    _registry.dropInactive(now);
    sendKeepAlives(now);
}

void ConnectionService::shutdown() {
    for (const net::Address& address : _registry.disconnectAll()) {
        send(address, net::Disconnect{});
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
        send(from, net::Refused{net::RefuseReason::WrongVersion});
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
        send(from, *reply);
    }
}

void ConnectionService::sendKeepAlives(Clock::time_point now) {
    for (const net::Address& address : _registry.takeKeepAliveTargets(now)) {
        send(address, net::KeepAlive{});
    }
}

void ConnectionService::send(const net::Address& to, const net::ControlMessage& message) {
    const auto packet = net::buildControlPacket(message);
    (void)_socket.send(to, packet); // UDP : un échec d'envoi équivaut à une perte
}

} // namespace odyssee::server
