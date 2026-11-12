"""Torche : la première lumière, avant la lampe (docs/GAMEPLAY.md : à la main). Brûle un temps.

Référence : art/concepts/objets/outils-et-ressources.png (torche à droite).
Bâton de bois brut, tête entourée de tissu imbibé, ligaturée de corde, flammes et braises au bout.
Les flammes émettent de la lumière ; dans Unreal, particules et lumière vacillante.
Debout, 0,55 m (flammes comprises).
"""
from lowpoly.fire import AMBER, ORANGE, YELLOW, flame
from lowpoly.materials import flat_material
from lowpoly.tool_parts import cloth_band, haft, lashing

ICON = True
VIEW = (0.3, -1.0, 0.2)
PREVIEW_ROTATION_DEG = (0, 15, 0)

LENGTH = 0.44
HEAD = 0.36           # début de la tête en tissu
CHAR = (0.20, 0.16, 0.13)


def build():
    parts, center_x, radius_at = haft("torche", LENGTH, 0.017, 0.02, bend=0.008, seed=8,
                                      bare=(0.08, 0.6))
    parts += cloth_band("tissu", center_x, lambda z: radius_at(z) + 0.02, HEAD, LENGTH, seed=8,
                        flatten=1.0)  # tête épaisse : du tissu enroulé et imbibé
    parts += lashing("ligature", center_x, radius_at, HEAD - 0.01, HEAD + 0.04, extra=0.012)
    top = (center_x(LENGTH), 0.0, LENGTH - 0.01)
    for i, (color, height, width, angle, tilt) in enumerate(
            ((ORANGE, 0.2, 0.12, 0.0, 0.0), (AMBER, 0.15, 0.09, 35.0, 14.0),
             (AMBER, 0.13, 0.08, -40.0, -14.0), (YELLOW, 0.1, 0.06, 70.0, 6.0))):
        parts += flame(f"flamme_{i}", color, height, width, angle, top, tilt)
    return parts
