// Implémentation POSIX (Linux, macOS) de UdpSocket. La version Windows (Winsock) arrive avec T019.
#include "odyssee/net/UdpSocket.hpp"

#include "SockaddrPosix.hpp"
#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace odyssee::net {

namespace {

constexpr log::Logger kLog{"net"};

const char* lastError() { return std::strerror(errno); }

// --- Étapes de UdpSocket::open() -------------------------------------------------------------

std::optional<int> createSocket() {
    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        kLog.error("socket() a échoué : {}", lastError());
        return std::nullopt;
    }
    return fd;
}

bool setNonBlocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        kLog.error("passage en non bloquant impossible : {}", lastError());
        return false;
    }
    return true;
}

bool bindToPort(int fd, std::uint16_t port) {
    const sockaddr_in local = posix::anyInterface(port);
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) < 0) {
        kLog.error("bind() sur le port {} a échoué : {}", port, lastError());
        return false;
    }
    return true;
}

/// Relit le port réellement attribué : indispensable quand on a demandé le port 0.
std::optional<std::uint16_t> queryLocalPort(int fd) {
    sockaddr_in bound{};
    socklen_t length = sizeof(bound);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &length) < 0) {
        kLog.error("getsockname() a échoué : {}", lastError());
        return std::nullopt;
    }
    return posix::fromSockaddr(bound).port;
}

// --- Lecture ----------------------------------------------------------------------------------

/// Résultat d'une lecture : un datagramme, ou un datagramme trop gros (à ignorer).
struct RawDatagram {
    Address from;
    std::size_t size = 0;
    bool tooBig = false;
};

/// Lit un seul datagramme. std::nullopt : plus rien à lire (ou erreur, journalisée).
std::optional<RawDatagram> readOneDatagram(int fd, std::span<std::byte> buffer) {
    sockaddr_in from{};
    socklen_t length = sizeof(from);

    // Lecture dans un tampon d'un octet de plus que le plus gros datagramme accepté : s'il est
    // rempli, le datagramme était trop gros. Portable, contrairement à MSG_TRUNC, qui ne renvoie
    // la taille réelle que sous Linux (ailleurs, un datagramme tronqué passerait pour entier).
    std::array<std::byte, kMaxPacketSize + 1> scratch;
    const auto got = ::recvfrom(fd, scratch.data(), scratch.size(), 0,
                                reinterpret_cast<sockaddr*>(&from), &length);

    if (got < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            kLog.warn("recvfrom() a échoué : {}", lastError());
        }
        return std::nullopt;
    }

    const auto size = static_cast<std::size_t>(got);
    if (size > kMaxPacketSize || size > buffer.size()) {
        return RawDatagram{posix::fromSockaddr(from), size, true};
    }
    std::copy_n(scratch.begin(), size, buffer.begin());
    return RawDatagram{posix::fromSockaddr(from), size, false};
}

} // namespace

// --- Cycle de vie -----------------------------------------------------------------------------

std::optional<UdpSocket> UdpSocket::open(std::uint16_t port) {
    const auto fd = createSocket();
    if (!fd) {
        return std::nullopt;
    }

    // Dès maintenant, `socket` ferme fd automatiquement si une étape échoue (RAII).
    UdpSocket socket(*fd, 0);

    if (!setNonBlocking(*fd) || !bindToPort(*fd, port)) {
        return std::nullopt;
    }

    const auto localPort = queryLocalPort(*fd);
    if (!localPort) {
        return std::nullopt;
    }

    socket._port = *localPort;
    kLog.debug("socket UDP ouvert sur le port {}", socket._port);
    return socket;
}

UdpSocket::~UdpSocket() { close(); }

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
    : _handle(std::exchange(other._handle, kInvalidHandle)),
      _port(std::exchange(other._port, std::uint16_t{0})),
      _oversized(std::exchange(other._oversized, std::uint64_t{0})) {}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        close();
        _handle = std::exchange(other._handle, kInvalidHandle);
        _port = std::exchange(other._port, std::uint16_t{0});
        _oversized = std::exchange(other._oversized, std::uint64_t{0});
    }
    return *this;
}

void UdpSocket::close() {
    if (_handle != kInvalidHandle) {
        ::close(static_cast<int>(_handle));
        _handle = kInvalidHandle;
    }
}

// --- Envoi et réception -----------------------------------------------------------------------

bool UdpSocket::send(const Address& to, std::span<const std::byte> data) {
    if (data.size() > kMaxPacketSize) {
        kLog.warn("datagramme de {} octets refusé (max {})", data.size(), kMaxPacketSize);
        return false;
    }

    const sockaddr_in dest = posix::toSockaddr(to);
    const auto sent = ::sendto(static_cast<int>(_handle), data.data(), data.size(), 0,
                               reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));

    if (sent < 0) {
        // Tampon d'envoi plein ou erreur : en UDP, le paquet est simplement perdu.
        kLog.debug("sendto() vers {} a échoué : {}", to.toString(), lastError());
        return false;
    }
    return static_cast<std::size_t>(sent) == data.size();
}

std::optional<Received> UdpSocket::receive(std::span<std::byte> buffer) {
    // On saute les datagrammes trop gros (tronqués, donc corrompus) sans s'arrêter :
    // renvoyer std::nullopt ferait croire à l'appelant qu'il n'y a plus rien à lire.
    while (const auto datagram = readOneDatagram(static_cast<int>(_handle), buffer)) {
        if (!datagram->tooBig) {
            return Received{datagram->from, datagram->size};
        }
        ++_oversized; // compté, pas journalisé : voir oversizedDatagrams()
    }
    return std::nullopt;
}

} // namespace odyssee::net
