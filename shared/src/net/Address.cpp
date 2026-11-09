#include "odyssee/net/Address.hpp"

#include <charconv>
#include <format>

namespace odyssee::net {

std::optional<Address> Address::parse(std::string_view text, std::uint16_t port) {
    // Analyse à la main (pas d'inet_pton) : code portable, sans en-tête système,
    // et strict : exactement 4 nombres de 0 à 255 séparés par des points.
    std::uint32_t ip = 0;
    const char* it = text.data();
    const char* const end = text.data() + text.size();

    for (int octet = 0; octet < 4; ++octet) {
        if (octet > 0) {
            if (it == end || *it != '.') {
                return std::nullopt;
            }
            ++it;
        }
        unsigned value = 0;
        const auto [next, ec] = std::from_chars(it, end, value);
        const auto digits = next - it;
        if (ec != std::errc{} || value > 255 || digits > 3 || (digits > 1 && *it == '0')) {
            return std::nullopt; // vide, trop grand, ou zéro en tête ("01") refusé
        }
        ip = (ip << 8) | value;
        it = next;
    }
    if (it != end) {
        return std::nullopt; // caractères en trop
    }
    return Address{ip, port};
}

std::string Address::toString() const {
    return std::format("{}.{}.{}.{}:{}", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF,
                       ip & 0xFF, port);
}

} // namespace odyssee::net
