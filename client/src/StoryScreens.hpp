#pragma once

#include "StoryState.hpp"

#include "odyssee/data/StoryCatalog.hpp"

namespace odyssee::debugclient {

/// Les écrans de l'histoire (T049) dans le client de debug, en texte simple par-dessus le jeu :
/// choix du survivant (F1 à F4), écran du réveil, indice lu, message, journal (touche J).
/// Tout ce qui est visible à l'instant `now` (secondes, GetTime de raylib).
void drawStory(const data::StoryCatalog& story, const StoryState& state, double now);

} // namespace odyssee::debugclient
