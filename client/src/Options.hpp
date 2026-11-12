#pragma once

#include "odyssee/core/Config.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace odyssee::debugclient {

/// Options du client de debug, issues de la ligne de commande. Ce qui est donné remplace les
/// réglages enregistrés de l'écran de connexion.
struct Options {
    std::optional<std::string> host; ///< adresse IPv4 : donnée, on se connecte sans l'écran
    std::optional<std::uint16_t> port;
    std::optional<std::string> name;
    std::optional<std::string> inviteCode;
    /// Fichier monde : le même que celui du serveur. Vide : `odyssee.world` à côté de
    /// l'exécutable s'il y en a un, sinon sol plat provisoire.
    std::string worldPath;
};

/// Lit `[--name Nom] [--invite code] [--world fichier] [adresse] [port]` (sans le nom du
/// programme). std::nullopt si un argument est inconnu ou invalide.
[[nodiscard]] std::optional<Options> parseOptions(std::span<const std::string_view> args);

/// Texte d'aide affiché quand les arguments sont invalides.
[[nodiscard]] std::string usage(std::string_view programName);

} // namespace odyssee::debugclient
