#include "Hud.hpp"

#include <raylib.h>

#include <cstdio>
#include <string>

namespace odyssee::debugclient {

namespace {

constexpr int kLine = 22;

void line(int row, const std::string& text, Color color = RAYWHITE) {
    DrawText(text.c_str(), 16, 16 + row * kLine, 20, color);
}

std::string positionText(const HudInfo& info) {
    if (!info.x) {
        return "Position : en attente du serveur";
    }
    char text[96];
    std::snprintf(text, sizeof text, "Position : %.2f  %.2f  %.2f m", static_cast<double>(*info.x),
                  static_cast<double>(*info.y), static_cast<double>(*info.z));
    return text;
}

void drawStamina(int row, float stamina, bool exhausted) {
    const int y = 16 + row * kLine;
    DrawRectangle(16, y, 200, 14, Color{0, 0, 0, 120});
    DrawRectangle(16, y, static_cast<int>(200 * stamina), 14, exhausted ? RED : GREEN);
    DrawText("endurance", 224, y - 2, 18, RAYWHITE);
}

} // namespace

void drawHud(const HudInfo& info) {
    line(0, "Odyssée - client de debug - " + info.connection);
    line(1, info.playerId ? "Joueur n° " + std::to_string(*info.playerId) : "Joueur : -");
    line(2, info.pingMs ? "Ping : " + std::to_string(*info.pingMs) + " ms" : "Ping : -");
    line(3, positionText(info));
    line(4, "Autres joueurs visibles : " + std::to_string(info.playersInView));
    drawStamina(5, info.stamina, info.exhausted);
    DrawFPS(16, 16 + 6 * kLine);

    const int bottom = GetScreenHeight() - 30;
    DrawText(info.mouseCaptured ? "ZQSD/WASD bouger - Maj courir - Espace sauter - Ctrl accroupi"
                                  " - V vue extérieure - Échap libérer la souris"
                                : "Clic : reprendre la souris",
             16, bottom, 18, Color{255, 255, 255, 200});
}

} // namespace odyssee::debugclient
