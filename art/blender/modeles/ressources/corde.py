"""Corde (ressource) : tressée à partir des fibres. Ligatures, arcs, pièges, échelles.

Pas de concept dédié : le style des ligatures des outils (lowpoly/tool_parts.py). Une corde enroulée
en écheveau, nouée en son milieu par quelques tours, un bout qui pend. Posée à plat, ~25 cm.
"""
import math

from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.tool_parts import material

ICON = True
VIEW = (0.3, -1.0, 0.8)

ROPE = 0.011          # rayon de la corde
LOOPS = 6             # tours de l'écheveau
RADII = (0.12, 0.085)  # demi-axes d'un tour (écheveau ovale)


ROPE_COLOR = (0.70, 0.58, 0.40)   # chanvre clair : corde neuve, pas une ligature usée


def _shade(part, seed):
    return two_tone(part, ROPE_COLOR, tuple(c * 0.75 for c in ROPE_COLOR),
                    "corde", dark_side=(0.3, 0.6, -0.8), threshold=0.1, seed=seed)


def _loops():
    """Les tours de l'écheveau, empilés et un peu décalés."""
    loops = []
    for i in range(LOOPS):
        shift = (i - LOOPS / 2) * 0.006
        ring = [(math.cos(a) * (RADII[0] + shift), math.sin(a) * (RADII[1] - shift * 0.5),
                 ROPE + (i % 3) * ROPE * 1.4) for a in (math.tau * k / 14 for k in range(15))]
        loops.append(_shade(tube(f"tour_{i}", ring, [ROPE] * len(ring), material("corde"), sides=6),
                            seed=i))
    return loops


def _binding():
    """Quelques tours serrés qui tiennent l'écheveau, d'un côté."""
    parts = []
    for i in range(3):
        x = RADII[0] + 0.005 - i * 0.014
        ring = [(x, -0.035 * math.cos(a), 0.03 + 0.03 * math.sin(a))
                for a in (math.tau * k / 8 for k in range(9))]
        parts.append(_shade(tube(f"noeud_{i}", ring, [ROPE * 0.9] * 9, material("corde"), sides=5),
                            seed=10 + i))
    return parts


def _loose_end():
    """Le bout libre qui dépasse et traîne au sol."""
    end = [(-RADII[0] * 0.7, -RADII[1] * 0.75, ROPE), (-RADII[0] - 0.03, -RADII[1] - 0.04, ROPE),
           (-RADII[0] - 0.09, -RADII[1] - 0.02, ROPE), (-RADII[0] - 0.13, -RADII[1] - 0.06, ROPE)]
    return [_shade(tube("bout", end, [ROPE, ROPE, ROPE, ROPE * 0.8], material("corde"), sides=6),
                   seed=20)]


def build():
    return _loops() + _binding() + _loose_end()
