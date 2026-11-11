// Client de debug d'Odyssée (raylib) : se connecte au serveur et prédit le joueur local (T031).
// Le vrai client est Unreal (plugin OdysseeNet) ; celui-ci sert à tester le réseau sous Linux.
#include "DebugClient.hpp"
#include "Options.hpp"

#include "odyssee/client/ServerLink.hpp"
#include "odyssee/net/Address.hpp"

#include <raylib.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace odyssee;

namespace {

void openWindow() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Odyssée - client de debug");
    SetExitKey(KEY_NULL); // Échap libère la souris ; on quitte en fermant la fenêtre
    DisableCursor();
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    const auto options = debugclient::parseOptions(args);
    if (!options) {
        std::cerr << debugclient::usage(argv[0]);
        return 1;
    }

    const auto server = net::Address::parse(options->host, options->port);
    if (!server) {
        std::cerr << "Adresse invalide : " << options->host << " (IPv4 attendue)\n";
        return 1;
    }
    auto link = client::ServerLink::open(options->name, *server);
    if (!link) {
        std::cerr << "Impossible d'ouvrir un socket UDP\n";
        return 1;
    }

    openWindow();
    {
        debugclient::DebugClient game(std::move(*link));
        game.run();
    }
    CloseWindow();
    return 0;
}
