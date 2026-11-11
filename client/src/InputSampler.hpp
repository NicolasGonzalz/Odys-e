#pragma once

#include "odyssee/net/GameMessages.hpp"

namespace odyssee::debugclient {

/// Lit le clavier et la souris (raylib) et en fait l'input du tick.
///
/// Touches physiques : ZQSD en AZERTY = WASD en QWERTY (raylib/GLFW désignent des positions de
/// touche). Maj : courir ; Espace : sauter ; Ctrl : s'accroupir ; E : utiliser ; clics : actions.
/// La souris tourne la caméra à chaque image (update), l'input en prend l'état au tick (sample).
class InputSampler {
  public:
    /// À chaque image : la souris tourne la caméra (si elle est capturée).
    void update();

    /// L'input du tick : direction, cap et inclinaison de la caméra, boutons enfoncés.
    [[nodiscard]] net::InputCommand sample() const;

    /// Cap en degrés : 0 = est, 90 = sud (comme la simulation).
    [[nodiscard]] float yawDegrees() const { return _yaw; }
    /// Inclinaison en degrés : positive vers le haut.
    [[nodiscard]] float pitchDegrees() const { return _pitch; }

    /// Reprend le cap donné par le serveur (au démarrage de la prédiction).
    void lookTowards(float yawDegrees) { _yaw = yawDegrees; }

  private:
    float _yaw = 0.0f;
    float _pitch = 0.0f;
};

} // namespace odyssee::debugclient
