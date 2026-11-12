#include "PauseMenu.hpp"

#include "Widgets.hpp"

#include <raylib.h>

namespace odyssee::debugclient {

PauseChoice drawPauseMenu() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 150});
    const float width = 300.0f;
    const float x = (static_cast<float>(GetScreenWidth()) - width) / 2.0f;
    const float top = static_cast<float>(GetScreenHeight()) / 2.0f - 110.0f;

    drawCentered("Pause", static_cast<int>(top) - 60, 34, RAYWHITE);
    if (button({x, top, width, 48}, "Reprendre")) {
        return PauseChoice::Resume;
    }
    if (button({x, top + 64, width, 48}, "Se déconnecter")) {
        return PauseChoice::Disconnect;
    }
    if (button({x, top + 128, width, 48}, "Quitter le jeu")) {
        return PauseChoice::Quit;
    }
    drawCentered("Échap : reprendre", static_cast<int>(top) + 200, 18, LIGHTGRAY);
    return PauseChoice::None;
}

} // namespace odyssee::debugclient
