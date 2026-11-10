"""Hache en pierre (niveau 1) : l'outil du début, fabriqué avec les moyens du bord.

Style : art/concepts/ambiances/zone-1-plaine.jpg (hache trapue en main : manche épais, grosse tête
facettée, tissu enroulé sous la tête). Pièces et couleurs communes : lowpoly/tool_parts.py.
Pierre taillée vers -x, glissée dans le manche fendu, ligaturée en corde.
"""
from lowpoly.tool_parts import cloth_band, grip, haft, knapped, lashing

ICON = True
VIEW = (0.55, -1.0, 0.35)

LENGTH = 0.68
HEAD_Z = 0.56

# Stations de la pierre : (x, demi-épaisseur, demi-hauteur, centre z). Talon épais à droite
# (il dépasse du manche), tranchant fin, haut et légèrement tombant à gauche.
STONE = [
    (0.050, 0.024, 0.036, HEAD_Z + 0.006),
    (0.000, 0.034, 0.052, HEAD_Z),
    (-0.065, 0.029, 0.066, HEAD_Z - 0.006),
    (-0.135, 0.017, 0.080, HEAD_Z - 0.014),
    (-0.195, 0.005, 0.088, HEAD_Z - 0.022),
]


def build():
    parts, center_x, radius_at = haft("manche", LENGTH, 0.029, 0.024, seed=1, bare=(0.3, 0.56))
    head = knapped("tete", STONE, seed=3)
    tie = lashing("ligature", center_x, radius_at, HEAD_Z - 0.07, HEAD_Z + 0.065, extra=0.015)
    band = cloth_band("bande", center_x, radius_at, HEAD_Z - 0.17, HEAD_Z - 0.075, seed=4)
    hand = grip("poignee", center_x, radius_at, 0.03, 0.19, seed=1)
    return parts + [head] + tie + band + hand
