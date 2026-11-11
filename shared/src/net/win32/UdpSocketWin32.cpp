// Implémentation Windows (Winsock) de UdpSocket. Même comportement que net/posix/.
#include "odyssee/net/UdpSocket.hpp"

#include "SockaddrWin32.hpp"
#include "WinsockHeaders.hpp"
#include "WinsockRuntime.hpp"
#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"

#include <mstcpip.h>
#include <utility>

// Selon la version du SDK Windows, mstcpip.h n'expose pas toujours cette constante.
// Sa valeur est fixe et documentée par Microsoft : on la définit si elle manque.
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

namespace odyssee::net {

namespace {

constexpr log::Logger kLog{"net"};

int lastError() { return ::WSAGetLastError(); }

/// Winsock attend des tailles en `int`, alors que sizeof donne un size_t.
template <typename T> constexpr int sizeAsInt() { return static_cast<int>(sizeof(T)); }

SOCKET toSocket(std::intptr_t handle) { return static_cast<SOCKET>(handle); }

// --- Étapes de UdpSocket::open() -------------------------------------------------------------

std::optional<SOCKET> createSocket() {
    const SOCKET socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket == INVALID_SOCKET) {
        kLog.error("socket() a échoué (code {})", lastError());
        return std::nullopt;
    }
    return socket;
}

bool setNonBlocking(SOCKET socket) {
    u_long nonBlocking = 1;
    if (::ioctlsocket(socket, FIONBIO, &nonBlocking) != 0) {
        kLog.error("passage en non bloquant impossible (code {})", lastError());
        return false;
    }
    return true;
}

/// Particularité Windows : un paquet envoyé vers un port fermé fait échouer le recvfrom() suivant
/// avec WSAECONNRESET. Pour un serveur UDP, c'est un piège : on désactive ce comportement.
void ignoreConnectionResets(SOCKET socket) {
    BOOL report = FALSE;
    DWORD returned = 0;
    ::WSAIoctl(socket, SIO_UDP_CONNRESET, &report, static_cast<DWORD>(sizeof(report)), nullptr, 0,
               &returned, nullptr, nullptr);
}

/// Empêche un autre programme de s'attacher au même port (sécurité, comportement proche de Linux).
void claimPortExclusively(SOCKET socket) {
    BOOL exclusive = TRUE;
    ::setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive),
                 sizeAsInt<BOOL>());
}

bool bindToPort(SOCKET socket, std::uint16_t port) {
    const sockaddr_in local = win32::anyInterface(port);
    if (::bind(socket, reinterpret_cast<const sockaddr*>(&local), sizeAsInt<sockaddr_in>()) != 0) {
        kLog.error("bind() sur le port {} a échoué (code {})", port, lastError());
        return false;
    }
    return true;
}

/// Relit le port réellement attribué : indispensable quand on a demandé le port 0.
std::optional<std::uint16_t> queryLocalPort(SOCKET socket) {
    sockaddr_in bound{};
    int length = sizeAsInt<sockaddr_in>();
    if (::getsockname(socket, reinterpret_cast<sockaddr*>(&bound), &length) != 0) {
        kLog.error("getsockname() a échoué (code {})", lastError());
        return std::nullopt;
    }
    return win32::fromSockaddr(bound).port;
}

// --- Lecture ----------------------------------------------------------------------------------

/// Résultat d'une lecture : un datagramme, ou un datagramme trop gros (déjà jeté par Windows).
struct RawDatagram {
    Address from;
    std::size_t size = 0;
    bool tooBig = false;
};

/// Lit un seul datagramme. std::nullopt : plus rien à lire (ou erreur, journalisée).
std::optional<RawDatagram> readOneDatagram(SOCKET socket, std::span<std::byte> buffer) {
    sockaddr_in from{};
    int length = sizeAsInt<sockaddr_in>();

    const int got = ::recvfrom(socket, reinterpret_cast<char*>(buffer.data()),
                               static_cast<int>(buffer.size()), 0,
                               reinterpret_cast<sockaddr*>(&from), &length);

    if (got != SOCKET_ERROR) {
        return RawDatagram{win32::fromSockaddr(from), static_cast<std::size_t>(got), false};
    }

    const int error = lastError();
    if (error == WSAEMSGSIZE) {
        // Contrairement à Linux (MSG_TRUNC), Windows ne donne pas la taille réelle.
        return RawDatagram{win32::fromSockaddr(from), buffer.size(), true};
    }
    if (error != WSAEWOULDBLOCK) {
        kLog.warn("recvfrom() a échoué (code {})", error);
    }
    return std::nullopt;
}

} // namespace

// --- Cycle de vie -----------------------------------------------------------------------------

std::optional<UdpSocket> UdpSocket::open(std::uint16_t port) {
    if (!win32::ensureWinsockStarted()) {
        return std::nullopt;
    }

    const auto socket = createSocket();
    if (!socket) {
        return std::nullopt;
    }

    // Dès maintenant, `owner` ferme le socket automatiquement si une étape échoue (RAII).
    UdpSocket owner(static_cast<Handle>(*socket), 0);

    ignoreConnectionResets(*socket);
    claimPortExclusively(*socket);
    if (!setNonBlocking(*socket) || !bindToPort(*socket, port)) {
        return std::nullopt;
    }

    const auto localPort = queryLocalPort(*socket);
    if (!localPort) {
        return std::nullopt;
    }

    owner._port = *localPort;
    kLog.debug("socket UDP ouvert sur le port {}", owner._port);
    return owner;
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
        ::closesocket(toSocket(_handle));
        _handle = kInvalidHandle;
    }
}

// --- Envoi et réception -----------------------------------------------------------------------

bool UdpSocket::send(const Address& to, std::span<const std::byte> data) {
    if (data.size() > kMaxPacketSize) {
        kLog.warn("datagramme de {} octets refusé (max {})", data.size(), kMaxPacketSize);
        return false;
    }

    const sockaddr_in dest = win32::toSockaddr(to);
    const int sent = ::sendto(toSocket(_handle), reinterpret_cast<const char*>(data.data()),
                              static_cast<int>(data.size()), 0,
                              reinterpret_cast<const sockaddr*>(&dest), sizeAsInt<sockaddr_in>());

    if (sent == SOCKET_ERROR) {
        // Tampon d'envoi plein ou erreur : en UDP, le paquet est simplement perdu.
        kLog.debug("sendto() vers {} a échoué (code {})", to.toString(), lastError());
        return false;
    }
    return static_cast<std::size_t>(sent) == data.size();
}

std::optional<Received> UdpSocket::receive(std::span<std::byte> buffer) {
    // On saute les datagrammes trop gros sans s'arrêter : renvoyer std::nullopt ferait croire
    // à l'appelant qu'il n'y a plus rien à lire.
    while (const auto datagram = readOneDatagram(toSocket(_handle), buffer)) {
        if (!datagram->tooBig) {
            return Received{datagram->from, datagram->size};
        }
        ++_oversized; // compté, pas journalisé : voir oversizedDatagrams()
    }
    return std::nullopt;
}

} // namespace odyssee::net
