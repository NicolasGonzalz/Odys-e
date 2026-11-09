#pragma once

#include "odyssee/core/Config.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace odyssee::server {

/// Options du serveur, issues de la ligne de commande.
struct Options {
    std::uint16_t port = kDefaultPort;
    bool verbose = false;
};

/// Lit les arguments (sans le nom du programme) : `[--verbose] [port]`, dans n'importe quel ordre.
/// Renvoie std::nullopt si un argument est inconnu, si le port est invalide ou donné deux fois.
[[nodiscard]] std::optional<Options> parseCommandLine(std::span<const std::string_view> args);

/// Texte d'aide affiché quand les arguments sont invalides.
[[nodiscard]] std::string usage(std::string_view programName);

} // namespace odyssee::server
