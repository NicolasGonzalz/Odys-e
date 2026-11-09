#include "Server.hpp"

#include "odyssee/core/Config.hpp"

#include <csignal>
#include <cstdlib>
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
    if (argc > 1) {
        try {
            const int parsed = std::stoi(argv[1]);
            if (parsed <= 0 || parsed > 65535) {
                throw std::out_of_range("port");
            }
            port = static_cast<std::uint16_t>(parsed);
        } catch (const std::exception&) {
            std::cerr << "Usage : " << argv[0] << " [port]\n";
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
