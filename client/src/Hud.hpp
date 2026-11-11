#pragma once

#include <optional>
#include <string>

namespace odyssee::debugclient {

/// Ce que le HUD de debug affiche (T034 l'enrichira : pertes, débit, corrections).
struct HudInfo {
    std::string connection;           ///< état de la connexion, en clair
    std::optional<int> playerId;
    std::optional<int> pingMs;        ///< aller-retour mesuré par la session
    std::optional<float> x, y, z;     ///< position prédite, en mètres
    float stamina = 0.0f;             ///< 0 à 1
    bool exhausted = false;
    int playersInView = 0;
    int corrections = 0;              ///< recalages sur le serveur (T032)
    float lastCorrectionCm = 0.0f;
    bool mouseCaptured = false;
};

/// Textes de debug et aide des touches, en surimpression.
void drawHud(const HudInfo& info);

} // namespace odyssee::debugclient
