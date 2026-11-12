#pragma once

#include <string>

namespace odyssee::debugclient {

/// Ce que le joueur tape sur l'écran de connexion. Gardé d'une fois sur l'autre dans un petit
/// fichier texte à côté de l'exécutable : on ne retape pas l'adresse du serveur à chaque partie.
struct ConnectSettings {
    std::string host = "127.0.0.1"; ///< adresse IPv4 du serveur
    std::string port;               ///< texte du champ (vide : port par défaut)
    std::string name = "Joueur";
    std::string inviteCode; ///< vide si le serveur n'en demande pas

    friend bool operator==(const ConnectSettings&, const ConnectSettings&) = default;
};

/// Réglages enregistrés (ou ceux par défaut si le fichier manque ou est illisible).
[[nodiscard]] ConnectSettings loadSettings(const std::string& path);

/// Enregistre les réglages. Faux si le fichier n'a pas pu être écrit.
bool saveSettings(const ConnectSettings& settings, const std::string& path);

/// Fichier des réglages : à côté de l'exécutable.
[[nodiscard]] std::string settingsPath();

} // namespace odyssee::debugclient
