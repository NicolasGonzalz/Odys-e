// Point d'entrée du serveur : lit les options, prépare le serveur, puis le lance.
#include "CommandLine.hpp"
#include "Server.hpp"
#include "ShutdownSignals.hpp"
#include "WorldLoading.hpp"

#include "odyssee/core/Log.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

/// La carte demandée, ou le sol plat provisoire si aucune ne l'est. std::nullopt si la carte
/// demandée n'a pas pu être chargée (la raison est dans le journal).
std::optional<odyssee::server::GameWorld> prepareWorld(const odyssee::server::Options& options) {
    if (options.worldPath.empty()) {
        return odyssee::server::GameWorld{};
    }
    return odyssee::server::loadGameWorld(options.worldPath);
}

} // namespace

int main(int argc, char** argv) {
    using namespace odyssee;

    const std::vector<std::string_view> args(argv + 1, argv + argc);
    const auto options = server::parseCommandLine(args);
    if (!options) {
        std::cerr << server::usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (options->verbose) {
        log::setLevel(log::Level::Debug);
    }

    auto world = prepareWorld(*options);
    if (!world) {
        return EXIT_FAILURE;
    }

    server::Server gameServer(options->port, options->network, std::move(*world));
    server::stopOnShutdownSignals(gameServer);
    return gameServer.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
