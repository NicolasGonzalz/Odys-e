#pragma once

namespace odyssee::debugclient {

/// Ce que le joueur choisit dans le menu Échap.
enum class PauseChoice {
    None,       ///< rien cette image : le menu reste ouvert
    Resume,     ///< retour au jeu
    Disconnect, ///< quitter la partie, retour à l'écran de connexion
    Quit,       ///< fermer le client
};

/// Dessine le menu Échap par-dessus la scène (entre BeginDrawing et EndDrawing) et renvoie le
/// bouton cliqué. Le jeu continue derrière : le serveur ne s'arrête pas pour un joueur.
[[nodiscard]] PauseChoice drawPauseMenu();

} // namespace odyssee::debugclient
