// Interne à shared/ : conversion entre net::Address et la structure Winsock sockaddr_in.
// C'est le seul endroit où l'on passe de l'ordre « machine » à l'ordre « réseau » (big-endian).
#pragma once

#include "WinsockHeaders.hpp"
#include "odyssee/net/Address.hpp"

namespace odyssee::net::win32 {

inline sockaddr_in toSockaddr(const Address& address) {
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(address.port);
    sa.sin_addr.s_addr = htonl(address.ip);
    return sa;
}

inline Address fromSockaddr(const sockaddr_in& sa) {
    return Address{ntohl(sa.sin_addr.s_addr), ntohs(sa.sin_port)};
}

/// Adresse « toutes les interfaces » (0.0.0.0) sur `port`, utilisée pour bind().
inline sockaddr_in anyInterface(std::uint16_t port) {
    return toSockaddr(Address{INADDR_ANY, port});
}

} // namespace odyssee::net::win32
