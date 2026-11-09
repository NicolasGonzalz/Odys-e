#pragma once

#include <string_view>

namespace odyssee::net {

/// Un pseudo valide : 1 à kMaxPlayerNameLength octets, sans caractère de contrôle
/// (retour à la ligne, tabulation…), et pas uniquement des espaces.
[[nodiscard]] bool isValidPlayerName(std::string_view name);

} // namespace odyssee::net
