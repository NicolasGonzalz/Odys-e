"""Marteau de charpentier (à panne fendue), le même partout : l'outil du joueur (construction, en
mode bac à sable) et celui posé sur l'établi.

Construit debout (manche selon z, pied à l'origine, tête en haut, face de frappe vers +x), dans
le style des outils (lowpoly/tool_parts.py) : manche en bois avec son fil, tête en acier usé,
face de frappe plus claire (polie par les coups), plaques de rouille.
"""
from .materials import flat_material
from .profile import profile_solid
from .shading import blotches, two_tone
from .shapes import box
from .tool_parts import TOOL_COLORS, haft, material

LENGTH = 0.4         # manche (m) : plus grand que nature, pour se lire en main (vue 1re pers.)
HEAD_SCALE = 1.25    # tête plus massive, même raison
HEAD_THICKNESS = 0.024 * HEAD_SCALE

# Profil de la tête vu de côté (x, z) autour du haut du manche : face de frappe à droite,
# panne fendue qui plonge vers l'arrière à gauche.
HEAD = [(0.064, 0.017), (0.064, -0.017), (0.042, -0.019), (0.02, -0.014), (-0.018, -0.014),
        (-0.04, -0.022), (-0.066, -0.044), (-0.078, -0.04), (-0.05, -0.006), (-0.022, 0.015),
        (0.04, 0.019)]


def _thickness(x):
    """Plus épaisse côté frappe (la face est large), fine au bout de la panne."""
    if x > 0.04 * HEAD_SCALE:
        return HEAD_THICKNESS * 1.25
    if x < -0.04 * HEAD_SCALE:
        return HEAD_THICKNESS * 0.7
    return HEAD_THICKNESS


def _head(top, seed):
    outline = [(x * HEAD_SCALE, top + z * HEAD_SCALE) for x, z in HEAD]
    head = profile_solid("marteau_tete", outline, _thickness, [material("metal")], lambda _: 0,
                         cuts_x=tuple(c * HEAD_SCALE for c in (-0.04, -0.02, 0.02, 0.042)),
                         chamfer=0.002)
    two_tone(head, TOOL_COLORS["metal"], TOOL_COLORS["metal_sombre"], "marteau_metal",
             dark_side=(0.2, 0.5, -1.0), threshold=0.2, seed=seed)
    head.data.materials.append(material("metal_clair"))
    for polygon in head.data.polygons:  # face de frappe, polie par les coups
        if polygon.normal.x > 0.8:
            polygon.material_index = len(head.data.materials) - 1
    return blotches(head, material("rouille"), count=2, radius=0.012, seed=seed)


def hammer(seed=0):
    """Toutes les pièces du marteau, debout (voir l'en-tête)."""
    parts, _, _ = haft("marteau_manche", LENGTH, 0.0165, 0.0135, bend=0.004, seed=seed,
                       bare=(0.15, 0.8))
    top = LENGTH - 0.022 * HEAD_SCALE
    parts.append(_head(top, seed))
    parts.append(box("marteau_coin", (0.006, 0.02, 0.006),  # coin qui bloque la tête
                     flat_material("marteau_coin", TOOL_COLORS["metal_sombre"]),
                     location=(0.0, 0.0, top + 0.02 * HEAD_SCALE)))
    return parts
