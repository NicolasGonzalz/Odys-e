#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace odyssee::net {

/// Adresse IPv4 + port. L'IP est stockée dans l'ordre « machine » :
/// 127.0.0.1 vaut 0x7F000001. La conversion vers l'ordre réseau (big-endian)
/// est faite uniquement par UdpSocket, au moment d'appeler le système.
struct Address {
    std::uint32_t ip = 0;
    std::uint16_t port = 0;

    /// 127.0.0.1:<port>, la machine locale.
    static constexpr Address loopback(std::uint16_t port) { return {0x7F000001u, port}; }

    /// Lit une IPv4 en notation pointée ("192.168.1.20").
    /// Renvoie std::nullopt si le texte est invalide.
    [[nodiscard]] static std::optional<Address> parse(std::string_view ip, std::uint16_t port);

    /// "192.168.1.20:27015"
    [[nodiscard]] std::string toString() const;

    friend bool operator==(const Address&, const Address&) = default;
};

} // namespace odyssee::net
