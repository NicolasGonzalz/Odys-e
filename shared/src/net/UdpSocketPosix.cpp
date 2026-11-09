// Implémentation POSIX (Linux, macOS) de UdpSocket. La version Windows (Winsock) arrive avec T019.
#include "odyssee/net/UdpSocket.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace odyssee::net {

namespace {

constexpr log::Logger kLog{"net"};

sockaddr_in toSockaddr(const Address& address) {
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(address.port);       // ordre machine → ordre réseau
    sa.sin_addr.s_addr = htonl(address.ip);
    return sa;
}

Address fromSockaddr(const sockaddr_in& sa) {
    return Address{ntohl(sa.sin_addr.s_addr), ntohs(sa.sin_port)}; // ordre réseau → ordre machine
}

bool wouldBlock(int error) { return error == EAGAIN || error == EWOULDBLOCK; }

} // namespace

std::optional<UdpSocket> UdpSocket::open(std::uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        kLog.error("socket() a échoué : {}", std::strerror(errno));
        return std::nullopt;
    }
    // À partir d'ici, `socket` ferme fd automatiquement si on sort en erreur.
    UdpSocket socket(fd, 0);

    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        kLog.error("passage en non bloquant impossible : {}", std::strerror(errno));
        return std::nullopt;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY); // toutes les interfaces
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) < 0) {
        kLog.error("bind() sur le port {} a échoué : {}", port, std::strerror(errno));
        return std::nullopt;
    }

    // Relire le port réellement attribué (indispensable quand port == 0).
    sockaddr_in bound{};
    socklen_t length = sizeof(bound);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &length) < 0) {
        kLog.error("getsockname() a échoué : {}", std::strerror(errno));
        return std::nullopt;
    }
    socket._port = ntohs(bound.sin_port);
    kLog.debug("socket UDP ouvert sur le port {}", socket._port);
    return socket;
}

UdpSocket::~UdpSocket() { close(); }

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
    : _handle(std::exchange(other._handle, kInvalidHandle)), _port(std::exchange(other._port, 0)) {}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        close();
        _handle = std::exchange(other._handle, kInvalidHandle);
        _port = std::exchange(other._port, 0);
    }
    return *this;
}

void UdpSocket::close() {
    if (_handle != kInvalidHandle) {
        ::close(static_cast<int>(_handle));
        _handle = kInvalidHandle;
    }
}

bool UdpSocket::send(const Address& to, std::span<const std::byte> data) {
    if (data.size() > kMaxPacketSize) {
        kLog.warn("datagramme de {} octets refusé (max {})", data.size(), kMaxPacketSize);
        return false;
    }
    const sockaddr_in dest = toSockaddr(to);
    const auto sent = ::sendto(static_cast<int>(_handle), data.data(), data.size(), 0,
                               reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
    if (sent < 0) {
        // Tampon d'envoi plein (EWOULDBLOCK) ou erreur : en UDP, le paquet est simplement perdu.
        kLog.debug("sendto() vers {} a échoué : {}", to.toString(), std::strerror(errno));
        return false;
    }
    return static_cast<std::size_t>(sent) == data.size();
}

std::optional<Received> UdpSocket::receive(std::span<std::byte> buffer) {
    while (true) {
        sockaddr_in from{};
        socklen_t length = sizeof(from);
        // MSG_TRUNC (Linux) : renvoie la taille réelle du datagramme, même s'il dépasse le tampon.
        const auto got = ::recvfrom(static_cast<int>(_handle), buffer.data(), buffer.size(),
                                    MSG_TRUNC, reinterpret_cast<sockaddr*>(&from), &length);
        if (got < 0) {
            if (!wouldBlock(errno)) {
                kLog.warn("recvfrom() a échoué : {}", std::strerror(errno));
            }
            return std::nullopt; // plus rien à lire (ou erreur)
        }
        const auto size = static_cast<std::size_t>(got);
        if (size > buffer.size()) {
            kLog.warn("datagramme de {} octets ignoré (tampon de {})", size, buffer.size());
            continue; // passer au datagramme suivant
        }
        return Received{fromSockaddr(from), size};
    }
}

} // namespace odyssee::net
