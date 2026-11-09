#include "odyssee/net/PlayerName.hpp"

#include "odyssee/core/Config.hpp"

#include <algorithm>

namespace odyssee::net {

namespace {

/// Caractères de contrôle ASCII (0–31 et 127). Les octets ≥ 128 (lettres accentuées en UTF-8)
/// sont acceptés.
bool isControlCharacter(char c) {
    const auto byte = static_cast<unsigned char>(c);
    return byte < 32 || byte == 127;
}

} // namespace

bool isValidPlayerName(std::string_view name) {
    if (name.empty() || name.size() > kMaxPlayerNameLength) {
        return false;
    }
    if (std::ranges::any_of(name, isControlCharacter)) {
        return false;
    }
    return std::ranges::any_of(name, [](char c) { return c != ' '; });
}

} // namespace odyssee::net
