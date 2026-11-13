#include "CommandLine.hpp"

#include "odyssee/net/InviteCode.hpp"

#include <charconv>
#include <format>

namespace odyssee::server {

namespace {

/// Lit un entier non signé occupant tout le texte ; tout autre texte est refusé.
/// std::from_chars ne lance pas d'exception, contrairement à std::stoi.
std::optional<unsigned> parseUnsigned(std::string_view text) {
    unsigned value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

/// Lit un port entre 1 et 65535.
std::optional<std::uint16_t> parsePort(std::string_view text) {
    const auto value = parseUnsigned(text);
    if (!value || *value == 0 || *value > 65535) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(*value);
}

/// Lit une durée en millisecondes (10 s au plus : au-delà, ce n'est plus un réseau).
std::optional<std::chrono::milliseconds> parseMilliseconds(std::string_view text) {
    const auto value = parseUnsigned(text);
    if (!value || *value > 10'000) {
        return std::nullopt;
    }
    return std::chrono::milliseconds{*value};
}

/// Lit un pourcentage entier entre 0 et 100.
std::optional<double> parsePercent(std::string_view text) {
    const auto value = parseUnsigned(text);
    if (!value || *value > 100) {
        return std::nullopt;
    }
    return static_cast<double>(*value);
}

/// Applique une option de simulation réseau suivie de sa valeur. Faux si l'option est inconnue
/// ou la valeur invalide.
bool applyNetworkOption(std::string_view name, std::string_view value,
                        net::NetworkConditions& network) {
    if (name == "--lag" || name == "--jitter") {
        const auto delay = parseMilliseconds(value);
        if (!delay) {
            return false;
        }
        (name == "--lag" ? network.latency : network.jitter) = *delay;
        return true;
    }
    if (name == "--loss" || name == "--dup") {
        const auto percent = parsePercent(value);
        if (!percent) {
            return false;
        }
        (name == "--loss" ? network.lossPercent : network.duplicatePercent) = *percent;
        return true;
    }
    return false;
}

bool isNetworkOption(std::string_view arg) {
    return arg == "--lag" || arg == "--jitter" || arg == "--loss" || arg == "--dup";
}

bool isTextOption(std::string_view arg) {
    return arg == "--world" || arg == "--data" || arg == "--invite";
}

/// Applique `--world fichier`, `--data dossier` ou `--invite code`. Faux si la valeur est vide
/// ou invalide.
bool applyTextOption(std::string_view name, std::string_view value, Options& options) {
    if (name == "--world" || name == "--data") {
        (name == "--world" ? options.worldPath : options.dataDir) = std::string(value);
        return !value.empty();
    }
    options.inviteCode = net::normalizeInviteCode(value);
    return !options.inviteCode.empty() && net::isValidInviteCode(value);
}

} // namespace

std::optional<Options> parseCommandLine(std::span<const std::string_view> args) {
    Options options;
    bool portGiven = false;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        if (arg == "--verbose") {
            options.verbose = true;
            continue;
        }
        if (isTextOption(arg)) {
            if (i + 1 >= args.size() || !applyTextOption(arg, args[i + 1], options)) {
                return std::nullopt; // valeur manquante ou invalide
            }
            ++i;
            continue;
        }
        if (isNetworkOption(arg)) {
            if (i + 1 >= args.size() || !applyNetworkOption(arg, args[i + 1], options.network)) {
                return std::nullopt; // valeur manquante ou invalide
            }
            ++i;
            continue;
        }

        const auto port = parsePort(arg);
        if (!port || portGiven) {
            return std::nullopt; // argument inconnu, port invalide, ou port en double
        }
        options.port = *port;
        portGiven = true;
    }
    return options;
}

std::string usage(std::string_view programName) {
    return std::format(
        "Usage : {} [--verbose] [--world fichier] [--data dossier] [--invite code] "
        "[--lag ms] [--jitter ms] [--loss %] [--dup %] [port]\n"
        "  --verbose      affiche les messages de debug\n"
        "  --world f      carte à charger (fichier .world) ; sans elle, sol plat\n"
        "  --data d       dossier des données (items.txt, resources.txt) ; défaut :\n"
        "                 « data » s'il existe, sinon pas de ressources\n"
        "  --invite code  code d'invitation exigé des joueurs (lettres et chiffres,\n"
        "                 32 au plus) ; sans lui, le serveur est ouvert à tous\n"
        "  --lag ms       simulation réseau : délai ajouté à chaque envoi\n"
        "  --jitter ms    simulation réseau : ± au hasard autour du délai\n"
        "  --loss %       simulation réseau : envois perdus (0 à 100)\n"
        "  --dup %        simulation réseau : envois en double (0 à 100)\n"
        "  port           port UDP d'écoute (défaut : {})\n",
        programName, kDefaultPort);
}

} // namespace odyssee::server
