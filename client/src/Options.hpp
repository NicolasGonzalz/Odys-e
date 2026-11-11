#pragma once

#include "odyssee/core/Config.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace odyssee::debugclient {

/// Options du client de debug, issues de la ligne de commande.
struct Options {
    std::string host = "127.0.0.1"; ///< adresse IPv4 du serveur
    std::uint16_t port = kDefaultPort;
    std::string name = "Joueur";
};

/// Lit `[--name Nom] [adresse] [port]` (sans le nom du programme). std::nullopt si un argument
/// est inconnu ou invalide.
[[nodiscard]] std::optional<Options> parseOptions(std::span<const std::string_view> args);

/// Texte d'aide affiché quand les arguments sont invalides.
[[nodiscard]] std::string usage(std::string_view programName);

} // namespace odyssee::debugclient
