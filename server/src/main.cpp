// Point d'entrée du serveur : lit les options, prépare le serveur, puis le lance.
#include "CommandLine.hpp"
#include "Server.hpp"
#include "ShutdownSignals.hpp"

#include "odyssee/core/Log.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

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

    server::Server gameServer(options->port);
    server::stopOnShutdownSignals(gameServer);
    return gameServer.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
