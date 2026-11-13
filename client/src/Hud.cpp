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

std::string vitalsText(const HudInfo& info) {
    if (!info.health) {
        return "Vie, faim, soif : en attente du serveur";
    }
    return "Vie " + std::to_string(*info.health) + " %  -  faim " + std::to_string(*info.hunger) +
           " %  -  soif " + std::to_string(*info.thirst) + " %";
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

std::string networkText(const HudInfo& info) {
    char text[120];
    std::snprintf(text, sizeof text,
                  "Ping : %s  -  pertes : %.0f %%  -  %.1f ko/s en haut, %.1f en bas",
                  info.pingMs ? (std::to_string(*info.pingMs) + " ms").c_str() : "-",
                  static_cast<double>(info.lossPercent), static_cast<double>(info.uploadKbps),
                  static_cast<double>(info.downloadKbps));
    return text;
}

std::string correctionText(const HudInfo& info) {
    if (info.corrections == 0) {
        return "Corrections : aucune";
    }
    char text[80];
    std::snprintf(text, sizeof text, "Corrections : %d (dernière : %.1f cm)", info.corrections,
                  static_cast<double>(info.lastCorrectionCm));
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
    line(2, networkText(info), info.lossPercent > 5.0f ? ORANGE : RAYWHITE);
    line(3, positionText(info));
    line(4, "Autres joueurs visibles : " + std::to_string(info.playersInView));
    line(5, correctionText(info));
    drawStamina(6, info.stamina, info.exhausted);
    const bool low = info.health && (*info.health < 30 || *info.hunger == 0 || *info.thirst == 0);
    line(8, vitalsText(info), low ? ORANGE : RAYWHITE);
    line(9, info.hotbar);
    DrawFPS(16, 16 + 7 * kLine);

    const int bottom = GetScreenHeight() - 30;
    DrawText(info.mouseCaptured ? "ZQSD/WASD bouger - Maj courir - Espace sauter - Ctrl accroupi"
                                  " - V vue - 1-5 outil - E manger"
                                  " - C artisanat - Échap menu"
                                : "Échap : menu - clic : reprendre la souris",
             16, bottom, 18, Color{255, 255, 255, 200});
}

} // namespace odyssee::debugclient
