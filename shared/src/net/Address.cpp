#include "odyssee/net/Address.hpp"

#include <charconv>
#include <format>

namespace odyssee::net {

namespace {

/// Résultat de la lecture d'un nombre de l'adresse : sa valeur et la position juste après.
struct Octet {
    std::uint32_t value = 0;
    const char* next = nullptr;
};

/// Lit un nombre de 0 à 255 (1 à 3 chiffres, sans zéro en tête comme « 01 »).
std::optional<Octet> parseOctet(const char* begin, const char* end) {
    unsigned value = 0;
    const auto [next, ec] = std::from_chars(begin, end, value);
    const auto digits = next - begin;

    const bool parsed = ec == std::errc{};
    const bool inRange = value <= 255 && digits <= 3;
    const bool leadingZero = digits > 1 && *begin == '0';

    if (!parsed || !inRange || leadingZero) {
        return std::nullopt;
    }
    return Octet{value, next};
}

/// Consomme le point qui sépare deux nombres.
std::optional<const char*> skipDot(const char* it, const char* end) {
    if (it == end || *it != '.') {
        return std::nullopt;
    }
    return it + 1;
}

} // namespace

std::optional<Address> Address::parse(std::string_view text, std::uint16_t port) {
    // Analyse à la main (pas d'inet_pton) : portable, sans en-tête système, et stricte.
    const char* it = text.data();
    const char* const end = text.data() + text.size();
    std::uint32_t ip = 0;

    for (int index = 0; index < 4; ++index) {
        if (index > 0) {
            const auto afterDot = skipDot(it, end);
            if (!afterDot) {
                return std::nullopt;
            }
            it = *afterDot;
        }

        const auto octet = parseOctet(it, end);
        if (!octet) {
            return std::nullopt;
        }
        ip = (ip << 8) | octet->value;
        it = octet->next;
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
