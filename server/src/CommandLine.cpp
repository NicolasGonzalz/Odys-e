#include "CommandLine.hpp"

#include <charconv>
#include <format>

namespace odyssee::server {

namespace {

/// Lit un port entre 1 et 65535 ; tout autre texte est refusé.
/// std::from_chars ne lance pas d'exception, contrairement à std::stoi.
std::optional<std::uint16_t> parsePort(std::string_view text) {
    unsigned value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);

    const bool wholeText = ec == std::errc{} && end == text.data() + text.size();
    if (!wholeText || value == 0 || value > 65535) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(value);
}

} // namespace

std::optional<Options> parseCommandLine(std::span<const std::string_view> args) {
    Options options;
    bool portGiven = false;

    for (const std::string_view arg : args) {
        if (arg == "--verbose") {
            options.verbose = true;
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
    return std::format("Usage : {} [--verbose] [port]\n"
                       "  --verbose  affiche les messages de debug\n"
                       "  port       port UDP d'écoute (défaut : {})\n",
                       programName, kDefaultPort);
}

} // namespace odyssee::server
