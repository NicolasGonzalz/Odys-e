"""Fibres (ressource) : herbes sèches et tiges ramassées dans la plaine. Elles deviennent corde et
tissu ; elles servent aussi d'allume-feu.

Référence : art/concepts/objets/outils-et-ressources.png (botte de fibres au centre).
Une botte de longues tiges sèches, serrée en son milieu par un lien, qui s'évase aux deux bouts.
Posée à plat, ~35 cm.
"""
import math
import random

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.tool_parts import material

ICON = True
VIEW = (0.3, -1.0, 0.8)

STRAW = [(0.80, 0.68, 0.40), (0.70, 0.58, 0.32), (0.86, 0.76, 0.48)]
LENGTH = 0.36
WAIST = 0.026     # demi-épaisseur de la botte au lien
FLARE = 0.1       # demi-largeur aux bouts (elle s'évase)
STRANDS = 56


def _strand(i, rng):
    """Une tige : serrée au lien (x = 0), écartée aux deux bouts, couchée sur le sol."""
    angle = rng.uniform(0, math.tau)
    r_end = FLARE * rng.uniform(0.4, 1.0)
    lift = rng.uniform(0.3, 1.0)
    points = []
    for x in (-LENGTH / 2 * rng.uniform(0.85, 1.0), -0.06, 0.0, 0.06, LENGTH / 2 * rng.uniform(0.85, 1.0)):
        spread = WAIST + (r_end - WAIST) * (abs(x) / (LENGTH / 2)) ** 1.5
        points.append((x, math.cos(angle) * spread, 0.02 + (math.sin(angle) * 0.5 + 0.5) * spread * lift))
    color = STRAW[i % len(STRAW)]
    return tube(f"tige_{i}", points, [0.0035] * 5, flat_material(f"fibre_{i % 3}", color), sides=3)


def _tie():
    ring = [(0.0, (WAIST + 0.006) * math.cos(a), 0.024 + (WAIST + 0.006) * math.sin(a))
            for a in (math.tau * k / 8 for k in range(9))]
    return [tube("lien", ring, [0.006] * 9, material("corde"), sides=5)]


def build():
    rng = random.Random(7)
    return [_strand(i, rng) for i in range(STRANDS)] + _tie()
