#pragma once

#include "ConnectSettings.hpp"
#include "Widgets.hpp"

#include "odyssee/net/Address.hpp"

#include <array>
#include <optional>
#include <string>

namespace odyssee::debugclient {

/// Ce que le joueur a choisi de rejoindre.
struct ConnectTarget {
    ConnectSettings settings;
    net::Address server;
};

/// Écran de connexion (T056) : adresse, port, pseudo et code d'invitation. Tab ou un clic change
/// de champ, Entrée ou le bouton se connecte. Les champs sont vérifiés avant de partir.
class ConnectScreen {
  public:
    /// `message` : affiché sous les champs (pourquoi on revient ici : refus, coupure…).
    ConnectScreen(ConnectSettings settings, std::string message);

    /// Tourne jusqu'au choix du joueur : où se connecter, ou std::nullopt pour quitter.
    [[nodiscard]] std::optional<ConnectTarget> run();

    /// Vérifie les champs sans rien afficher : la cible si tout est bon, sinon std::nullopt (et
    /// run() dira pourquoi).
    [[nodiscard]] std::optional<ConnectTarget> validate();

  private:
    static constexpr std::size_t kFieldCount = 4;

    [[nodiscard]] std::array<TextField, kFieldCount> fields();
    [[nodiscard]] std::array<Rectangle, kFieldCount> fieldBoxes() const;
    void handleFocus();
    void draw(bool& connectClicked, bool& quitClicked);

    ConnectSettings _settings;
    std::string _message;
    std::size_t _focus = 0;
};

} // namespace odyssee::debugclient
