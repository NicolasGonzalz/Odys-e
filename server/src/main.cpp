#include "Server.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/core/Log.hpp"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

odyssee::server::Server* gServer = nullptr;

void onSignal(int /*signal*/) {
    if (gServer != nullptr) {
        gServer->stop();
    }
}

} // namespace

int main(int argc, char** argv) {
    std::uint16_t port = odyssee::kDefaultPort;
    int argIndex = 1;
    if (argc > argIndex && std::strcmp(argv[argIndex], "--verbose") == 0) {
        odyssee::log::setLevel(odyssee::log::Level::Debug);
        ++argIndex;
    }
    if (argc > argIndex) {
        try {
            const int parsed = std::stoi(argv[argIndex]);
            if (parsed <= 0 || parsed > 65535) {
                throw std::out_of_range("port");
            }
            port = static_cast<std::uint16_t>(parsed);
        } catch (const std::exception&) {
            std::cerr << "Usage : " << argv[0] << " [--verbose] [port]\n";
            return EXIT_FAILURE;
        }
    }

    odyssee::server::Server server(port);
    gServer = &server;
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    server.run();
    return EXIT_SUCCESS;
}
