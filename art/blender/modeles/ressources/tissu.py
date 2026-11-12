"""Tissu (ressource) : tissé avec les fibres, ou récupéré dans les ruines. Bandages, sac de
couchage, torches, sacs.

Référence : art/concepts/objets/outils-et-ressources.png (rouleau de tissu).
Un rouleau de toile écrue lié d'une ficelle, dont un pan déroulé s'étale au sol, bord effiloché.
~30 cm.
"""
import math

from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.shapes import cylinder, roughen
from lowpoly.tool_parts import material

ICON = True
VIEW = (0.3, -1.0, 0.75)

CLOTH = (0.90, 0.83, 0.67)        # toile écrue, claire
CLOTH_DARK = (0.72, 0.65, 0.51)
ROLL_RADIUS = 0.05
ROLL_LENGTH = 0.24

# Pan déroulé vu de dessus (x, y) : il part du rouleau (x = 0) et s'étale, bord effiloché au bout.
FLAP = [(0.0, -0.11), (0.13, -0.13), (0.2, -0.1), (0.22, -0.06), (0.19, -0.02), (0.23, 0.02),
        (0.2, 0.07), (0.22, 0.11), (0.12, 0.13), (0.0, 0.11)]


def _roll():
    roll = cylinder("rouleau", ROLL_RADIUS, ROLL_LENGTH, flat_material("tissu_ecru", CLOTH),
                    vertices=10, location=(0, 0, ROLL_RADIUS), rotation_deg=(90, 0, 0))
    roughen(roll, 0.003, seed=2)
    two_tone(roll, CLOTH, CLOTH_DARK, "tissu_ecru", dark_side=(0.3, 0.6, -0.8), threshold=0.05,
             spread=0.04, seed=2)
    end = cylinder("rouleau_spirale", ROLL_RADIUS * 0.55, 0.004, flat_material("tissu_ombre",
                   CLOTH_DARK), vertices=8, location=(0, -ROLL_LENGTH / 2 - 0.001, ROLL_RADIUS),
                   rotation_deg=(90, 0, 0))
    return [roll, end]


def _flap():
    """Le pan déroulé, posé à plat contre le rouleau."""
    flat = [(x + ROLL_RADIUS * 0.6, -y) for x, y in FLAP]
    flap = profile_solid("pan", flat, lambda _: 0.006, [flat_material("tissu_ecru", CLOTH)],
                         lambda _: 0, cuts_x=(0.1, 0.18), chamfer=0.002)
    two_tone(flap, CLOTH, CLOTH_DARK, "tissu_pan", threshold=0.5, seed=3)
    return posed([flap], (90, 0, 0), (0, 0, 0.003))


def _string():
    """Ficelle nouée autour du rouleau."""
    ring = [(ROLL_RADIUS * 1.06 * math.cos(a), 0.04, ROLL_RADIUS + ROLL_RADIUS * 1.06 * math.sin(a))
            for a in (math.tau * k / 10 for k in range(11))]
    return [tube("ficelle", ring, [0.004] * 11, material("corde"), sides=4)]


def build():
    return _roll() + _flap() + _string()
