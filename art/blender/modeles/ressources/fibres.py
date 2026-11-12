"""Fibres (ressource) : longues feuilles fibreuses arrachées dans la plaine (lin, chanvre redevenus
sauvages). Elles deviennent corde et tissu ; sèches, elles servent d'allume-feu.

Référence : art/concepts/objets/outils-et-ressources.png (botte de fibres). Pas de la paille :
une botte de feuilles plates et souples, vert-sauge aux pointes sèches, qui retombent aux deux
bouts ; liée à deux endroits par une ficelle. Posée à plat, ~30 cm.
"""
import math
import random

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.tool_parts import material

ICON = True
VIEW = (0.3, -1.0, 0.8)

LEAVES = [(0.56, 0.63, 0.38), (0.48, 0.56, 0.32), (0.62, 0.66, 0.42)]  # vert-sauge
DRY = (0.70, 0.66, 0.42)                                                # pointes sèches
LENGTH = 0.3
COUNT = 22
TIES = (-0.06, 0.06)   # x des deux liens
WAIST = 0.022          # demi-épaisseur de la botte aux liens


def _blade_path(rng):
    """Ligne d'une feuille : serrée aux liens, bombée au milieu, ses bouts retombent au sol en
    s'écartant (chaque feuille un peu différente)."""
    angle = rng.uniform(0, math.tau)
    spread = rng.uniform(0.035, 0.07)
    half = LENGTH / 2 * rng.uniform(0.8, 1.0)
    points = []
    for t in (-1.0, -0.6, -0.2, 0.2, 0.6, 1.0):
        x = t * half
        flare = WAIST + (spread - WAIST) * max(0.0, abs(t) - 0.2) / 0.8
        lift = 0.035 * (1 - t * t)                     # bombée au milieu, posée aux bouts
        points.append((x, math.cos(angle) * flare, 0.012 + lift + (math.sin(angle) * 0.5 + 0.5)
                       * flare * 0.6 * (1 - abs(t))))
    return points


# Demi-épaisseur le long d'une feuille (× FLAT en largeur) : effilée aux deux bouts.
PROFILE = [0.0004, 0.0024, 0.003, 0.003, 0.0024, 0.0004]
FLAT = 3.6


def _blade(i, rng):
    """Une feuille plate et effilée ; seules ses pointes sont sèches, plus claires."""
    points = _blade_path(rng)
    color = LEAVES[i % len(LEAVES)]
    leaf = tube(f"feuille_{i}", points, PROFILE, flat_material(f"fibre_{i % 3}", color), sides=4,
                flatten=FLAT)
    dry = flat_material("fibre_seche", DRY)
    leaf.data.materials.append(dry)
    tip = LENGTH / 2 * 0.82
    for polygon in leaf.data.polygons:
        if abs(polygon.center.x) > tip:
            polygon.material_index = 1
    return leaf


def _ties():
    """Deux liens de ficelle, quelques tours chacun."""
    twine = material("corde")
    parts = []
    for i, x in enumerate(TIES):
        for k in range(2):
            ring = [(x + k * 0.007, (WAIST + 0.007) * math.cos(a),
                     0.045 + (WAIST + 0.007) * math.sin(a) * 0.9)
                    for a in (math.tau * j / 8 for j in range(9))]
            parts.append(tube(f"lien_{i}_{k}", ring, [0.0035] * 9, twine, sides=4))
    return parts


def build():
    rng = random.Random(12)
    return [_blade(i, rng) for i in range(COUNT)] + _ties()
