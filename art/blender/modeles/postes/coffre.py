"""Coffre en bois : ranger ce qu'on ne porte pas (et, en coop, les coffres communs de la base).
Fabriqué à l'établi.

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/coffre.png).
Planches horizontales, couvercle bombé fait de planches, deux bandes de fer par-dessus, montants
d'angle plus sombres, serrure et trou de clé, un peu de rouille.
0,8 × 0,5 m, 0,6 m de haut. Pivot au pied.

À faire pour l'animation (ouvrir / fermer) : exporter le couvercle à part, pivot sur la charnière
(arrière, à HEIGHT). Pour l'instant, tout est fusionné en un objet.
"""
import math

from lowpoly.carpentry import board
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.organic import band
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.shapes import box
from lowpoly.tool_parts import TOOL_COLORS, material

ICON = True
VIEW = (-0.6, -1.0, 0.55)

LENGTH, DEPTH, HEIGHT = 0.8, 0.5, 0.38   # corps (sans le couvercle)
LID_RADIUS = 0.27
LID_CENTER = HEIGHT - 0.1               # centre de l'arc du couvercle (sous le bord)
LID_ANGLES = (-55, -28, 0, 28, 55)      # planches du couvercle, en degrés depuis le sommet
BANDS_X = (-0.24, 0.24)
IRON = (0.36, 0.36, 0.38)
IRON_DARK = (0.24, 0.24, 0.26)


def _body():
    """Corps : trois rangs de planches sur chaque face, montants d'angle sombres."""
    parts = [box("coffre_coeur", (LENGTH - 0.04, DEPTH - 0.04, HEIGHT - 0.02),
                 flat_material("coffre_interieur", (0.25, 0.17, 0.11)), location=(0, 0, HEIGHT / 2))]
    for row in range(3):
        z = 0.065 + row * 0.125
        for side in (-1, 1):
            front = board(f"face_{row}_{side}", LENGTH - 0.06, 0.12, 0.024, tint=1.12 + row * 0.04,
                          seed=row * 4 + side + 2, grain=1)
            # Rotation de signe opposé au côté : la face du dessus de la planche regarde dehors.
            parts += posed(front, (-90 * side, 0, 0), (0, side * (DEPTH / 2 - 0.006), z))
            flank = board(f"cote_{row}_{side}", DEPTH - 0.06, 0.12, 0.024, tint=0.9 + row * 0.05,
                          seed=row * 4 + side + 20, grain=1)
            parts += posed(flank, (90, 0, 90 * side), (side * (LENGTH / 2 - 0.006), 0, z))
    for sx in (-1, 1):
        for sy in (-1, 1):
            post = board(f"montant_{sx}_{sy}", HEIGHT, 0.06, 0.06, ("poutre", "poutre_sombre"),
                         seed=30 + sx + 2 * sy, grain=0)
            parts += posed(post, (0, 90, 0), (sx * (LENGTH / 2 - 0.02), sy * (DEPTH / 2 - 0.02),
                                              HEIGHT / 2))
    return parts


def _arc(radius, steps=8):
    """Points de l'arc du couvercle (y, z), d'un bord à l'autre."""
    a0 = math.radians(LID_ANGLES[0] - 14)
    a1 = math.radians(LID_ANGLES[-1] + 14)
    return [(radius * math.sin(a0 + (a1 - a0) * i / steps),
             LID_CENTER + radius * math.cos(a0 + (a1 - a0) * i / steps)) for i in range(steps + 1)]


def _lid():
    """Couvercle bombé : planches posées sur un arc, flasques pleines aux deux bouts."""
    parts = []
    for i, angle in enumerate(LID_ANGLES):
        a = math.radians(angle)
        plank = board(f"couvercle_{i}", LENGTH + 0.02, 0.15, 0.03, tint=1.0 + (i % 2) * 0.06,
                      seed=40 + i, grain=2)
        parts += posed(plank, (-angle, 0, 0),
                       (0, LID_RADIUS * math.sin(a), LID_CENTER + LID_RADIUS * math.cos(a)))
    for side in (-1, 1):
        outline = [(y, z) for y, z in _arc(LID_RADIUS)] + [(_arc(LID_RADIUS)[-1][0], HEIGHT),
                                                           (_arc(LID_RADIUS)[0][0], HEIGHT)]
        flank = profile_solid(f"flasque_{side}", outline, lambda _: 0.03, [material("bois")],
                              lambda _: 0, chamfer=0.004)
        two_tone(flank, (0.60, 0.42, 0.27), (0.45, 0.31, 0.20), "coffre_flasque", seed=50 + side)
        parts += posed([flank], (0, 0, 90), (side * (LENGTH / 2 - 0.005), 0, 0))
    return parts


def _iron(part, seed):
    return two_tone(part, IRON, IRON_DARK, "coffre_fer", dark_side=(0.3, 0.6, -1.0),
                    threshold=0.1, seed=seed)


def _bands():
    """Deux bandes de fer : sur le devant, par-dessus le couvercle, et sur l'arrière."""
    parts = []
    for i, x in enumerate(BANDS_X):
        over = [(x, y, z) for y, z in _arc(LID_RADIUS + 0.018, steps=10)]
        parts.append(_iron(band(f"bande_couvercle_{i}", over, 0.05, 0.008, material("metal")),
                           seed=60 + i))
        for side in (-1, 1):
            strap = box(f"bande_{i}_{side}", (0.05, 0.008, HEIGHT), material("metal"),
                        location=(x, side * (DEPTH / 2 + 0.014), HEIGHT / 2))
            parts.append(_iron(strap, seed=62 + i + side))
    rim = box("bande_bord", (LENGTH + 0.02, 0.008, 0.035), material("metal"),
              location=(0, -(DEPTH / 2 + 0.014), HEIGHT - 0.02))
    return parts + [_iron(rim, seed=66)]


def _lock():
    """Serrure : plaque de fer et trou de clé."""
    y = -(DEPTH / 2 + 0.02)
    plate = box("serrure", (0.09, 0.012, 0.11), material("metal_sombre"),
                location=(0, y, HEIGHT - 0.03))
    keyhole = box("serrure_trou", (0.014, 0.004, 0.03), flat_material("trou", (0.04, 0.04, 0.04)),
                  location=(0, y - 0.007, HEIGHT - 0.04))
    return [_iron(plate, 70), keyhole]


def build():
    return _body() + _lid() + _bands() + _lock()
