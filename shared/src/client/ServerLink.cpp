#include "odyssee/client/ServerLink.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/net/PacketKind.hpp"

#include <array>
#include <utility>
#include <variant>

namespace odyssee::client {

std::optional<ServerLink> ServerLink::open(std::string playerName, const net::Address& server) {
    auto socket = net::UdpSocket::open();
    if (!socket) {
        return std::nullopt;
    }
    return ServerLink(std::move(playerName), std::move(*socket), server);
}

ServerLink::ServerLink(std::string playerName, net::UdpSocket socket, const net::Address& server)
    : _connection(std::move(playerName)), _socket(std::move(socket)), _server(server) {}

void ServerLink::poll(Clock::time_point now) {
    sendControlMessages(now);
    openSessionOnceConnected();
    receiveAll(now);
    collectSnapshots();
    _connection.update(now);
}

void ServerLink::sendInputs(const net::InputBatch& batch, Clock::time_point now) {
    if (!connected() || !_session || batch.commands.empty()) {
        return;
    }

    _session->queueUnreliable(batch);
    sendDatagram(_session->buildPacket(now), now);
    _connection.onGamePacketSent(now);
}

std::vector<net::Snapshot> ServerLink::takeSnapshots() {
    return std::exchange(_snapshots, {});
}

void ServerLink::sendControlMessages(Clock::time_point now) {
    while (const auto message = _connection.nextOutgoing(now)) {
        sendDatagram(net::buildControlPacket(*message), now);
    }
}

void ServerLink::sendDatagram(std::span<const std::byte> bytes, Clock::time_point now) {
    if (_socket.send(_server, bytes)) {
        _traffic.onSent(bytes.size(), now);
    }
}

void ServerLink::openSessionOnceConnected() {
    if (!connected()) {
        _session.reset(); // une nouvelle connexion repartira d'une session neuve
        return;
    }
    if (!_session) {
        _session.emplace(*_connection.sessionToken());
        _session->acceptOnly(net::GameMessageType::Snapshot); // le serveur n'envoie que ça
    }
}

void ServerLink::receiveAll(Clock::time_point now) {
    std::array<std::byte, kMaxPacketSize> buffer{};
    while (const auto received = _socket.receive(buffer)) {
        if (received->from != _server) {
            continue; // seul le serveur a quelque chose à nous dire
        }
        _traffic.onReceived(received->size, now);
        handle(std::span(buffer).first(received->size), now);
    }
}

void ServerLink::handle(std::span<const std::byte> datagram, Clock::time_point now) {
    const auto packet = net::parsePacket(datagram);
    if (!packet) {
        return;
    }

    if (net::isControlPacketKind(packet->kind)) {
        if (const auto message = net::decodeControl(packet->kind, packet->body)) {
            _connection.onMessage(*message, now);
        }
        return;
    }
    handleGamePacket(*packet, now);
}

void ServerLink::handleGamePacket(const net::IncomingPacket& packet, Clock::time_point now) {
    if (_session && _session->receive(packet, now)) {
        _connection.onGamePacketReceived(now);
        _traffic.onGamePacket(packet.header.sequence, now);
    }
}

void ServerLink::collectSnapshots() {
    if (!_session) {
        return;
    }
    for (auto& message : _session->takeMessages()) {
        if (auto* snapshot = std::get_if<net::Snapshot>(&message)) {
            _snapshots.push_back(std::move(*snapshot));
        }
    }
}

} // namespace odyssee::client
