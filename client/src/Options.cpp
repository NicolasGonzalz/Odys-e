#include "Options.hpp"

#include <charconv>

namespace odyssee::debugclient {

namespace {

std::optional<std::uint16_t> parsePort(std::string_view text) {
    unsigned value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value == 0 || value > 65535) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(value);
}

/// Un argument libre : d'abord l'adresse, puis le port.
bool acceptPositional(Options& options, std::string_view arg, int& positional) {
    if (positional == 0) {
        options.host = std::string(arg);
    } else if (const auto port = parsePort(arg); positional == 1 && port) {
        options.port = *port;
    } else {
        return false;
    }
    ++positional;
    return true;
}

} // namespace

std::optional<Options> parseOptions(std::span<const std::string_view> args) {
    Options options;
    int positional = 0;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--name") {
            if (i + 1 >= args.size() || args[i + 1].empty()) {
                return std::nullopt;
            }
            options.name = std::string(args[++i]);
        } else if (args[i].starts_with("-") || !acceptPositional(options, args[i], positional)) {
            return std::nullopt;
        }
    }
    return options;
}

std::string usage(std::string_view programName) {
    return "Usage : " + std::string(programName) +
           " [--name Nom] [adresse IPv4] [port]\n"
           "  par défaut : 127.0.0.1 et le port " +
           std::to_string(kDefaultPort) + "\n";
}

} // namespace odyssee::debugclient
