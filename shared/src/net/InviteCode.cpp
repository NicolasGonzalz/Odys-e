#include "odyssee/net/InviteCode.hpp"

#include <algorithm>

namespace odyssee::net {

namespace {

bool isSeparator(char c) {
    return c == '-' || c == ' ';
}

bool isAlphanumeric(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

char toUpper(char c) {
    return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
}

} // namespace

std::string normalizeInviteCode(std::string_view code) {
    std::string normalized;
    for (const char c : code) {
        if (!isSeparator(c)) {
            normalized.push_back(toUpper(c));
        }
    }
    return normalized;
}

bool isValidInviteCode(std::string_view code) {
    const std::string normalized = normalizeInviteCode(code);
    return normalized.size() <= kMaxInviteCodeLength && std::ranges::all_of(normalized, isAlphanumeric);
}

bool inviteCodeMatches(std::string_view expected, std::string_view given) {
    const std::string candidate = normalizeInviteCode(given);
    // Toujours kMaxInviteCodeLength tours : ni la longueur ni le contenu ne changent la durée.
    unsigned difference = candidate.size() == expected.size() ? 0U : 1U;
    for (std::size_t i = 0; i < kMaxInviteCodeLength; ++i) {
        const char a = i < expected.size() ? expected[i] : '\0';
        const char b = i < candidate.size() ? candidate[i] : '\0';
        difference |= static_cast<unsigned>(static_cast<unsigned char>(a ^ b));
    }
    return difference == 0 && candidate.size() <= kMaxInviteCodeLength;
}

} // namespace odyssee::net
