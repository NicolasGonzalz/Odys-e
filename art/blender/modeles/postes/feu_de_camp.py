"""Feu de camp : se réchauffer, cuire, éclairer la nuit (et marquer un point de repos). Fabriqué à
la main (docs/GAMEPLAY.md § 2).

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/feu_de_camp.png).
Cercle de neuf grosses pierres claires, bûches en tipi au centre, flammes orange et jaunes,
cendres au sol. Flammes et braises émettent de la lumière ; l'animation des flammes viendra plus
tard (dans Unreal). Environ 1 m de diamètre.
"""
import math
import random

from mathutils import Vector

from lowpoly.carpentry import stain
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.rock import rock
from lowpoly.shading import two_tone
from lowpoly.shapes import cone
from lowpoly.tool_parts import TOOL_COLORS
from lowpoly.wood import end_grain, streaked

ICON = True
VIEW = (-0.4, -1.0, 0.75)

RING = 0.4        # rayon du cercle de pierres
STONES = 9
STONE = (0.78, 0.74, 0.68)     # pierres beiges et claires, comme sur le concept
STONE_DARK = (0.58, 0.55, 0.51)
ASH = (0.56, 0.53, 0.49)
BARK = (0.55, 0.38, 0.25)
BARK_DARK = (0.40, 0.27, 0.18)
# Langues de flamme : (couleur, rayon, hauteur, décalage x, décalage y, inclinaison en degrés).
FLAMES = [((1.0, 0.45, 0.10), 0.09, 0.42, 0.0, 0.0, 0.0),
          ((1.0, 0.55, 0.14), 0.06, 0.3, 0.06, 0.03, 14.0),
          ((1.0, 0.55, 0.14), 0.06, 0.26, -0.05, -0.04, -16.0),
          ((1.0, 0.80, 0.30), 0.045, 0.22, 0.02, -0.07, 10.0),
          ((1.0, 0.88, 0.45), 0.035, 0.2, -0.03, 0.07, -8.0)]
LOG_RINGS = [(0.3, TOOL_COLORS["bois_coeur"]), (1.0, TOOL_COLORS["bois_bout"])]


def _stones():
    rng = random.Random(4)
    stone = flat_material("feu_pierre", STONE)
    parts = []
    for i in range(STONES):
        angle = math.tau * i / STONES + rng.uniform(-0.12, 0.12)
        size = (0.12 * rng.uniform(0.85, 1.15), 0.1, 0.085 * rng.uniform(0.9, 1.2))
        piece = rock(f"pierre_{i}", size, stone, count=18, jitter=0.15, dents=1, base=0.7,
                     facets=0.3, seed=10 + i)
        piece.location = (math.cos(angle) * RING, math.sin(angle) * RING, size[2] * 0.7)
        piece.rotation_euler = (0, 0, angle)
        parts.append(two_tone(piece, STONE, STONE_DARK, "feu_pierre", dark_side=(0.3, 0.6, -1.0),
                              threshold=0.0, seed=10 + i))
    return parts


def _logs():
    """Quatre bûches en tipi : le pied sur la cendre, la tête vers le centre, au-dessus."""
    parts = []
    for i in range(4):
        angle = math.tau * i / 4 + 0.4
        foot = Vector((math.cos(angle) * 0.26, math.sin(angle) * 0.26, 0.04))
        head = Vector((math.cos(angle) * 0.03, math.sin(angle) * 0.03, 0.3))
        log = tube(f"buche_{i}", [tuple(foot), tuple((foot + head) / 2), tuple(head)],
                   [0.045, 0.042, 0.035], flat_material("feu_ecorce", BARK), sides=6)
        axis = (head - foot).normalized()
        streaked(log, BARK, BARK_DARK, f"feu_buche_{i}", axis=tuple(axis), seed=20 + i)
        cut = end_grain(f"buche_{i}_bout", tuple(foot), tuple(-axis), tuple(axis.orthogonal()),
                        (0.042, 0.042), LOG_RINGS, 6, lift=0.001, seed=i)
        parts += [log, *cut]
    return parts


def _flames():
    """Plusieurs langues de flamme séparées (orange au centre, jaunes autour), penchées chacune un
    peu de travers ; et quelques braises."""
    parts = []
    for i, (color, radius, height, dx, dy, tilt) in enumerate(FLAMES):
        flame = cone(f"flamme_{i}", radius, height, flat_material(f"feu_flamme_{i}", color,
                                                                  emission=4.0),
                     vertices=5, location=(dx, dy, 0.1 + height / 2),
                     rotation_deg=(tilt * 0.6, tilt, i * 37.0))
        parts.append(flame)
    rng = random.Random(7)
    for i in range(6):
        ember = rock(f"braise_{i}", (0.03, 0.025, 0.02),
                     flat_material("feu_braise", (1.0, 0.38, 0.1), emission=3.0), count=8, dents=0,
                     seed=40 + i)
        ember.location = (rng.uniform(-0.15, 0.15), rng.uniform(-0.15, 0.15), 0.02)
        parts.append(ember)
    return parts


def build():
    ash = stain("cendres", (0.0, 0.0, 0.002), RING * 0.85, ASH, seed=1)
    return [ash] + _stones() + _logs() + _flames()
