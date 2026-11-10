"""Pioche en pierre (niveau 1) : une pierre allongée à deux pointes, ligaturée en travers.

Même style que la hache en pierre (lowpoly/tool_parts.py) : manche épais, tissu sous la tête,
ligature en corde, poignée bandée. La pointe droite descend, comme sur la pioche du concept.
"""
from lowpoly.tool_parts import cloth_band, grip, haft, knapped, lashing

ICON = True
VIEW = (0.55, -1.0, 0.35)

LENGTH = 0.80
HEAD_Z = 0.70

# Stations : (x, demi-épaisseur, demi-hauteur, centre z), d'une pointe à l'autre.
STONE = [
    (-0.265, 0.006, 0.008, HEAD_Z - 0.014),
    (-0.180, 0.019, 0.023, HEAD_Z - 0.002),
    (-0.080, 0.030, 0.034, HEAD_Z + 0.005),
    (0.000, 0.034, 0.039, HEAD_Z + 0.005),
    (0.080, 0.030, 0.034, HEAD_Z),
    (0.170, 0.019, 0.023, HEAD_Z - 0.022),
    (0.245, 0.006, 0.008, HEAD_Z - 0.055),
]


def build():
    parts, center_x, radius_at = haft("manche", LENGTH, 0.030, 0.025, seed=2, bare=(0.26, 0.66))
    head = knapped("tete", STONE, seed=5)
    tie = lashing("ligature", center_x, radius_at, HEAD_Z - 0.06, HEAD_Z + 0.055, extra=0.012)
    band = cloth_band("bande", center_x, radius_at, HEAD_Z - 0.16, HEAD_Z - 0.065, seed=6)
    hand = grip("poignee", center_x, radius_at, 0.03, 0.19, seed=2)
    return parts + [head] + tie + band + hand
