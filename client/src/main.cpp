// Client de debug d'Odyssée (raylib) : se connecte au serveur et prédit le joueur local (T031).
// Le vrai client est Unreal (plugin OdysseeNet) ; celui-ci sert à tester le réseau sous Linux.
#include "ConnectScreen.hpp"
#include "ConnectSettings.hpp"
#include "DebugClient.hpp"
#include "Options.hpp"

#include "odyssee/client/ServerLink.hpp"
#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/Address.hpp"
#include "odyssee/world/WorldFileLoader.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <raylib.h>

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace odyssee;

namespace {

void openWindow() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Odyssée - client de debug");
    SetExitKey(KEY_NULL); // Échap ouvre le menu ; on quitte par le menu ou la fenêtre
}

/// Réglages enregistrés, remplacés par ce que la ligne de commande donne.
debugclient::ConnectSettings initialSettings(const debugclient::Options& options) {
    auto settings = debugclient::loadSettings(debugclient::settingsPath());
    settings.host = options.host.value_or(settings.host);
    settings.port = options.port ? std::to_string(*options.port) : settings.port;
    settings.name = options.name.value_or(settings.name);
    settings.inviteCode = options.inviteCode.value_or(settings.inviteCode);
    return settings;
}

/// Une partie avec ces réglages. Le message dit pourquoi elle s'est arrêtée, s'il y a lieu.
debugclient::SessionEnd playOnce(const debugclient::ConnectTarget& target,
                                 debugclient::GameData data) {
    auto link =
        client::ServerLink::open(target.settings.name, target.server, target.settings.inviteCode);
    if (!link) {
        return {false, "Impossible d'ouvrir un socket UDP"};
    }
    debugclient::DebugClient game(std::move(*link), data);
    return game.run();
}

/// Écran de connexion, partie, et retour à l'écran, jusqu'à ce que le joueur quitte. Avec une
/// adresse en ligne de commande, la première partie part sans passer par l'écran.
void play(const debugclient::Options& options, debugclient::GameData data) {
    auto settings = initialSettings(options);
    bool skipScreen = options.host.has_value();
    std::string message;
    while (true) {
        debugclient::ConnectScreen screen(settings, message);
        auto target = skipScreen ? screen.validate() : std::nullopt;
        target = target ? target : screen.run();
        skipScreen = false;
        if (!target) {
            return;
        }
        settings = target->settings;
        debugclient::saveSettings(settings, debugclient::settingsPath());
        const auto end = playOnce(*target, data);
        if (end.quit) {
            return;
        }
        message = end.message;
    }
}

/// Carte posée à côté de l'exécutable (paquet Windows de la CI) : prise sans --world.
std::string bundledMapPath() {
    const std::string path = std::string(GetApplicationDirectory()) + "odyssee.world";
    return FileExists(path.c_str()) ? path : std::string{};
}

/// La carte demandée, sinon celle posée à côté de l'exécutable (nul si aucune). std::nullopt
/// si elle n'a pas pu être chargée.
std::optional<std::unique_ptr<const world::WorldTerrain>> loadMap(std::string path) {
    if (path.empty()) {
        path = bundledMapPath();
    }
    if (path.empty()) {
        return std::unique_ptr<const world::WorldTerrain>{};
    }
    auto decoded = world::loadWorldFile(path);
    if (!decoded.world) {
        std::cerr << "Carte « " << path << " » non chargée : " << world::describe(decoded.error)
                  << "\n";
        return std::nullopt;
    }
    std::cout << "Carte : " << path << "\n";
    return std::make_unique<const world::WorldTerrain>(std::move(*decoded.world));
}

/// Types de ressources (T044), pour les dessiner : `data` du dossier courant, sinon à côté de
/// l'exécutable. Absents ou invalides : std::nullopt (formes neutres), sans empêcher de jouer.
std::optional<data::ResourceCatalog> loadResourceTypes() {
    const std::string besideExecutable = std::string(GetApplicationDirectory()) + "data";
    for (const std::string& dir : {std::string("data"), besideExecutable}) {
        if (!FileExists((dir + "/resources.txt").c_str())) {
            continue;
        }
        const auto items = data::Catalog::fromDirectory(dir);
        auto types = items.catalog ? data::ResourceCatalog::fromDirectory(dir, *items.catalog)
                                   : data::ResourceCatalogResult{};
        if (types.catalog) {
            std::cout << "Ressources : " << dir << "/resources.txt\n";
            return std::move(types.catalog);
        }
        const auto& error = items.error ? items.error : types.error;
        std::cerr << "Ressources non chargées : " << (error ? error->describe() : dir) << "\n";
    }
    return std::nullopt;
}

/// Objets et recettes (noms de l'inventaire) : --data, sinon data/ à côté de l'exécutable ou
/// du dossier courant. Absents : l'inventaire montre des numéros.
std::optional<data::Catalog> loadCatalog(std::string dir) {
    for (const std::string& candidate :
         {dir, std::string(GetApplicationDirectory()) + "data", std::string("data")}) {
        if (!candidate.empty() && FileExists((candidate + "/items.txt").c_str())) {
            dir = candidate;
            break;
        }
    }
    auto result = data::Catalog::fromDirectory(dir);
    if (!result.catalog) {
        const std::string why =
            result.error ? result.error->describe() : "dossier data/ introuvable";
        std::cerr << "Objets non chargés : " << why << "\n";
    }
    return std::move(result.catalog);
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    const auto options = debugclient::parseOptions(args);
    if (!options) {
        std::cerr << debugclient::usage(argv[0]);
        return 1;
    }
    auto map = loadMap(options->worldPath);
    if (!map) {
        return 1;
    }

    const auto resourceTypes = loadResourceTypes();
    const auto catalog = loadCatalog(options->dataDir);

    openWindow();
    play(*options, {map->get(), catalog ? &*catalog : nullptr,
                    resourceTypes ? &*resourceTypes : nullptr});
    CloseWindow();
    return 0;
}
