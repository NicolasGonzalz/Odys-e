"""Feu de camp : se réchauffer, cuire, éclairer la nuit (et marquer un point de repos). Fabriqué à
la main (docs/GAMEPLAY.md § 2).

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/feu_de_camp.png).
Cercle de neuf grosses pierres claires, six bûches en tipi et du petit bois, lit de braises,
flammes orange et jaunes et étincelles, cendres au sol ; broche de cuisson et marmite suspendue
(ajout au concept, pour l'ambiance survie). Flammes et braises émettent de la lumière ; l'animation des flammes viendra plus
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
from lowpoly.shapes import box, cone, cylinder
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
FLAMES = [((1.0, 0.45, 0.10), 0.11, 0.62, 0.0, 0.0, 0.0),
          ((1.0, 0.45, 0.10), 0.07, 0.42, -0.06, 0.05, -20.0),
          ((1.0, 0.55, 0.14), 0.06, 0.3, 0.06, 0.03, 14.0),
          ((1.0, 0.55, 0.14), 0.06, 0.26, -0.05, -0.04, -16.0),
          ((1.0, 0.80, 0.30), 0.045, 0.22, 0.02, -0.07, 10.0),
          ((1.0, 0.88, 0.45), 0.035, 0.2, -0.03, 0.07, -8.0)]
SPIT_HEIGHT = 1.0  # traverse de la broche
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
    """Six bûches en tipi (longueurs inégales) : le pied sur la cendre, la tête vers le centre."""
    parts = []
    for i in range(6):
        angle = math.tau * i / 6 + 0.3
        reach = 0.3 if i % 2 else 0.26
        foot = Vector((math.cos(angle) * reach, math.sin(angle) * reach, 0.04))
        # Têtes écartées du centre : le tipi reste ouvert, on voit les flammes entre les bûches.
        head = Vector((math.cos(angle) * 0.08, math.sin(angle) * 0.08, 0.28 + (i % 3) * 0.03))
        log = tube(f"buche_{i}", [tuple(foot), tuple((foot + head) / 2), tuple(head)],
                   [0.045, 0.042, 0.034], flat_material("feu_ecorce", BARK), sides=6)
        axis = (head - foot).normalized()
        streaked(log, BARK, BARK_DARK, f"feu_buche_{i}", axis=tuple(axis), seed=20 + i)
        cut = end_grain(f"buche_{i}_bout", tuple(foot), tuple(-axis), tuple(axis.orthogonal()),
                        (0.042, 0.042), LOG_RINGS, 6, lift=0.001, seed=i)
        parts += [log, *cut]
    return parts + _kindling()


def _kindling():
    """Petit bois éparpillé dans le cercle, prêt à rajouter."""
    rng = random.Random(11)
    twig = flat_material("feu_brindille", BARK_DARK)
    parts = []
    for i in range(7):
        angle = rng.uniform(0, math.tau)
        r = rng.uniform(0.2, 0.3)
        x, y = math.cos(angle) * r, math.sin(angle) * r
        turn = angle + math.pi / 2 + rng.uniform(-0.6, 0.6)
        dx, dy = math.cos(turn) * 0.07, math.sin(turn) * 0.07
        parts.append(tube(f"brindille_{i}", [(x - dx, y - dy, 0.012), (x + dx, y + dy, 0.015)],
                          [0.009, 0.007], twig, sides=4))
    return parts


def _coal_bed():
    """Lit de braises qui rougeoie sous les bûches, entouré de cendre."""
    return [stain("braises_lit", (0.0, 0.0, 0.004), 0.17, (0.95, 0.35, 0.08), seed=3)]


def _flames():
    """Plusieurs langues de flamme séparées (orange au centre, jaunes autour), penchées chacune un
    peu de travers ; des braises au sol et des étincelles qui montent."""
    parts = []
    for i, (color, radius, height, dx, dy, tilt) in enumerate(FLAMES):
        flame = cone(f"flamme_{i}", radius, height, flat_material(f"feu_flamme_{i}", color,
                                                                  emission=4.0),
                     vertices=5, location=(dx, dy, 0.1 + height / 2),
                     rotation_deg=(tilt * 0.6, tilt, i * 37.0))
        parts.append(flame)
    rng = random.Random(7)
    for i in range(8):
        ember = rock(f"braise_{i}", (0.03, 0.025, 0.02),
                     flat_material("feu_braise", (1.0, 0.38, 0.1), emission=3.0), count=8, dents=0,
                     seed=40 + i)
        ember.location = (rng.uniform(-0.16, 0.16), rng.uniform(-0.16, 0.16), 0.02)
        parts.append(ember)
    spark = flat_material("feu_etincelle", (1.0, 0.8, 0.35), emission=6.0)
    for i in range(9):
        parts.append(box(f"etincelle_{i}", (0.012, 0.012, 0.012), spark,
                         location=(rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12),
                                   rng.uniform(0.55, 0.85)), rotation_deg=(45, 45, i * 20)))
    return parts


def _spit():
    """Broche de cuisson : deux bâtons fourchus, une traverse, une marmite suspendue."""
    stick = flat_material("feu_broche", BARK)
    parts = []
    for side in (-1, 1):
        x = side * 0.52
        parts.append(tube(f"broche_poteau_{side}", [(x, 0.0, 0.0), (x, 0.0, SPIT_HEIGHT - 0.01)],
                          [0.02, 0.016], stick, sides=5))
        for branch in (-1, 1):  # la fourche
            parts.append(tube(f"broche_fourche_{side}_{branch}",
                              [(x, 0.0, SPIT_HEIGHT - 0.07), (x, branch * 0.05, SPIT_HEIGHT + 0.05)],
                              [0.012, 0.009], stick, sides=4))
    parts.append(tube("broche_traverse", [(-0.56, 0.0, SPIT_HEIGHT), (0.56, 0.0, SPIT_HEIGHT)],
                      [0.014, 0.014], stick, sides=5))
    return parts + _pot()


def _pot():
    """Marmite de fer noircie, pendue sous la traverse par une anse."""
    iron = flat_material("feu_marmite", (0.22, 0.22, 0.24))
    bottom = SPIT_HEIGHT - 0.3   # au-dessus des flammes, pas devant
    pot = cylinder("marmite", 0.08, 0.1, iron, vertices=10, location=(0, 0, bottom + 0.05),
                   radius_top=0.088)
    two_tone(pot, (0.26, 0.26, 0.28), (0.15, 0.15, 0.17), "feu_marmite", threshold=0.1, seed=5)
    handle = tube("marmite_anse", [(-0.085, 0.0, bottom + 0.1), (-0.03, 0.0, SPIT_HEIGHT - 0.01),
                                   (0.03, 0.0, SPIT_HEIGHT - 0.01), (0.085, 0.0, bottom + 0.1)],
                  [0.004] * 4, iron, sides=4)
    soup = cylinder("marmite_soupe", 0.077, 0.004, flat_material("feu_soupe", (0.55, 0.40, 0.22)),
                    vertices=10, location=(0, 0, bottom + 0.098))
    return [pot, handle, soup]


def build():
    ash = stain("cendres", (0.0, 0.0, 0.002), RING * 0.85, ASH, seed=1)
    return [ash] + _coal_bed() + _stones() + _logs() + _flames() + _spit()
