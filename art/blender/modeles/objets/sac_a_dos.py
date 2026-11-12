"""Sac à dos : il porte l'inventaire (une meilleure version = plus de place). Visible sur le dos des
survivants en coop, et posé au sol à la mort (« sac au sol », docs/VISION.md).

Pas sur la planche des objets : d'après celui de Teo (art/concepts/personnages/teo-brandt.jpg).
Grand sac en toile kaki usée, rabat sur le dessus fermé par deux sangles, poche avant, deux
bretelles rembourrées au dos, sac de couchage roulé et sanglé en dessous, rouleau de corde accroché
sur le côté, une pièce cousue sur un accroc. Debout, dos vers +y. 0,38 × 0,24 m, 0,58 m de haut.
"""
import math

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.shapes import bevel, box, cylinder, roughen
from lowpoly.tool_parts import material

ICON = True
VIEW = (-0.6, -1.0, 0.35)

CANVAS = (0.62, 0.54, 0.40)        # toile beige-kaki, délavée
CANVAS_DARK = (0.46, 0.39, 0.28)
STRAP = (0.30, 0.25, 0.19)
PATCH = (0.62, 0.52, 0.38)
ROLL = (0.44, 0.47, 0.37)
WIDTH, DEPTH, HEIGHT = 0.36, 0.22, 0.46
BOTTOM = 0.12                       # le sac commence au-dessus du rouleau


def _canvas(part, seed):
    roughen(part, 0.004, seed=seed)  # toile qui n'est pas tendue
    return two_tone(part, CANVAS, CANVAS_DARK, "sac_toile", dark_side=(0.4, 0.5, -0.4),
                    threshold=0.1, spread=0.04, seed=seed)


def _body():
    """Corps arrondi, rabat sur le dessus, poche avant ; bevel généreux : c'est de la toile."""
    canvas = flat_material("sac_toile", CANVAS)
    body = bevel(box("sac", (WIDTH, DEPTH, HEIGHT), canvas, location=(0, 0, BOTTOM + HEIGHT / 2)),
                 0.06, segments=3)
    flap = bevel(box("rabat", (WIDTH + 0.02, DEPTH + 0.02, 0.1), canvas,
                     location=(0, -0.005, BOTTOM + HEIGHT - 0.02)), 0.04, segments=3)
    pocket = bevel(box("poche", (WIDTH * 0.7, 0.07, 0.18), canvas,
                       location=(0, -DEPTH / 2 - 0.025, BOTTOM + 0.16)), 0.03, segments=3)
    pocket_flap = bevel(box("poche_rabat", (WIDTH * 0.72, 0.08, 0.05),
                            flat_material("sac_toile_sombre", CANVAS_DARK),
                            location=(0, -DEPTH / 2 - 0.03, BOTTOM + 0.25)), 0.02, segments=2)
    patch = box("piece", (0.08, 0.004, 0.07), flat_material("sac_piece", PATCH),
                location=(0.11, -DEPTH / 2 - 0.004, BOTTOM + 0.36), rotation_deg=(0, 8, 0))
    return [_canvas(body, 1), _canvas(flap, 2), _canvas(pocket, 3), pocket_flap, patch]


def _straps():
    """Sangles du rabat (devant), bretelles rembourrées (dos), boucles."""
    strap = flat_material("sac_sangle", STRAP)
    parts = []
    for i, x in enumerate((-0.09, 0.09)):
        front = [(x, -0.02, BOTTOM + HEIGHT + 0.03), (x, -DEPTH / 2 - 0.02, BOTTOM + HEIGHT - 0.02),
                 (x, -DEPTH / 2 - 0.03, BOTTOM + HEIGHT - 0.14)]
        parts.append(tube(f"sangle_rabat_{i}", front, [0.009] * 3, strap, sides=4))
        parts.append(box(f"boucle_{i}", (0.03, 0.008, 0.025), material("metal_sombre"),
                         location=(x, -DEPTH / 2 - 0.035, BOTTOM + HEIGHT - 0.1)))
        back = [(x * 0.8, DEPTH / 2, BOTTOM + HEIGHT - 0.04), (x * 0.9, DEPTH / 2 + 0.05,
                BOTTOM + HEIGHT * 0.6), (x * 1.3, DEPTH / 2 + 0.035, BOTTOM + 0.04)]
        parts.append(tube(f"bretelle_{i}", back, [0.02, 0.022, 0.016], strap, sides=6))
    return parts


def _bedroll():
    """Sac de couchage roulé et sanglé, sous le sac."""
    roll = cylinder("rouleau", 0.075, WIDTH + 0.06, flat_material("sac_rouleau", ROLL), vertices=10,
                    location=(0, -0.01, 0.075), rotation_deg=(0, 90, 0))
    two_tone(roll, ROLL, tuple(c * 0.72 for c in ROLL), "sac_rouleau", seed=5)
    parts = [roll]
    for i, x in enumerate((-0.12, 0.12)):
        ring = [(x, 0.078 * math.cos(a) - 0.01, 0.075 + 0.078 * math.sin(a))
                for a in (math.tau * k / 8 for k in range(9))]
        parts.append(tube(f"rouleau_sangle_{i}", ring, [0.007] * 9,
                          flat_material("sac_sangle", STRAP), sides=4))
    return parts


def _rope():
    """Rouleau de corde accroché sur le côté droit."""
    rope = material("corde")
    parts = []
    for i in range(3):
        ring = [(WIDTH / 2 + 0.03 + 0.008 * i, 0.06 * math.cos(a), BOTTOM + 0.2 + 0.075 * math.sin(a))
                for a in (math.tau * k / 10 for k in range(11))]
        parts.append(tube(f"corde_{i}", ring, [0.007] * 11, rope, sides=4))
    return parts


def build():
    return _body() + _straps() + _bedroll() + _rope()
