#pragma once

#include "odyssee/core/Config.hpp"
#include "odyssee/net/NetworkSimulator.hpp"

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
    /// Fichier monde à charger (tools/worldgen/output/odyssee.world). Vide : sol plat provisoire.
    std::string worldPath;
    /// Dossier des données du jeu (items.txt, recipes.txt, resources.txt). Vide : `data/` à côté
    /// de l'exécutable, sinon `data/` du dossier courant (sinon : ni objets ni ressources).
    std::string dataDir;
    /// Code d'invitation exigé (T056), normalisé. Vide : serveur ouvert à tous.
    std::string inviteCode;
    net::NetworkConditions network; ///< simulation de mauvais réseau (aucune par défaut)
};

/// Lit les arguments (sans le nom du programme) : `[--verbose] [--world fichier] [--data dossier]
/// [--invite code]
/// [--lag ms] [--jitter ms] [--loss %] [--dup %] [port]`, dans n'importe quel ordre.
/// Renvoie std::nullopt si un argument est inconnu, une valeur invalide ou manquante, ou si le
/// port est donné deux fois.
[[nodiscard]] std::optional<Options> parseCommandLine(std::span<const std::string_view> args);

/// Texte d'aide affiché quand les arguments sont invalides.
[[nodiscard]] std::string usage(std::string_view programName);

} // namespace odyssee::server
