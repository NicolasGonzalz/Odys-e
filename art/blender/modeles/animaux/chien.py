"""Chien redevenu sauvage (faune revenue), type dobermann low-poly au style « cartoon ».

Référence : art/blender/references/chien.png (bestiaire, en haut à gauche).
Proportions du concept : grosse tête, corps trapu, pattes épaisses, cou court.
Pelage sombre, marques feu (museau, sourcils, poitrail, bas des pattes), grandes oreilles dressées.
Tourné vers +x. Hauteur au garrot ~0,6 m.
"""
from lowpoly.materials import flat_material
from lowpoly.organic import loft, mirrored, tube
from lowpoly.shapes import blob, cone

VIEW = (0.75, -1.0, 0.45)  # trois quarts avant gauche, un peu de haut, comme le concept

DARK = (0.17, 0.14, 0.16)
TAN = (0.72, 0.45, 0.22)
PINK = (0.75, 0.48, 0.45)
BLACK = (0.04, 0.04, 0.05)


def _dark():
    return flat_material("pelage_sombre", DARK)


def _tan():
    return flat_material("feu", TAN)


def _body():
    """Du bassin au garrot : (x, z centre, demi-largeur, demi-hauteur). Court et trapu."""
    body = loft("corps", [
        (-0.36, 0.58, 0.08, 0.09),
        (-0.30, 0.58, 0.15, 0.16),
        (-0.12, 0.58, 0.16, 0.17),
        (0.08, 0.60, 0.18, 0.21),
        (0.24, 0.62, 0.17, 0.23),   # poitrail profond
        (0.32, 0.68, 0.12, 0.16),
    ], _dark(), sides=8)
    chest = blob("poitrail_feu", 0.13, _tan(), subdivisions=1, scale=(0.55, 0.9, 1.0),
                 location=(0.3, 0, 0.5))
    return [body, chest]


def _neck_and_head():
    neck = tube("cou", [(0.26, 0, 0.7), (0.34, 0, 0.8), (0.38, 0, 0.86)], [0.13, 0.12, 0.11],
                _dark(), sides=8)
    head = loft("tete", [
        (0.29, 0.93, 0.08, 0.09),
        (0.38, 0.95, 0.145, 0.14),   # crâne large (style du concept)
        (0.49, 0.94, 0.13, 0.13),
        (0.55, 0.91, 0.09, 0.09),
    ], _dark(), sides=8)
    muzzle = loft("museau", [
        (0.52, 0.89, 0.085, 0.075),
        (0.63, 0.87, 0.068, 0.058),
        (0.72, 0.86, 0.05, 0.047),
    ], _tan(), sides=7)
    nose = blob("truffe", 0.04, flat_material("truffe", BLACK), subdivisions=1,
                location=(0.735, 0, 0.885))
    return [neck, head, muzzle, nose]


def _ear(side):
    outer = cone(f"oreille_{side}", 0.075, 0.23, _dark(), vertices=4,
                 location=(0.38, side * 0.09, 1.14), rotation_deg=(side * -16, -6, 0))
    inner = cone(f"oreille_int_{side}", 0.047, 0.16, flat_material("oreille", PINK), vertices=4,
                 location=(0.397, side * 0.085, 1.13), rotation_deg=(side * -16, -6, 0))
    return [outer, inner]


def _face_marks(side):
    eye = blob(f"oeil_{side}", 0.02, flat_material("oeil", BLACK), subdivisions=1,
               location=(0.52, side * 0.1, 0.97))
    brow = blob(f"sourcil_{side}", 0.024, _tan(), subdivisions=1,
                location=(0.51, side * 0.105, 1.01))
    return [eye, brow]


def _front_leg(side):
    """Patte avant épaisse : épaule → coude → poignet → pied ; le bas est couleur feu."""
    y = side * 0.11
    upper = tube(f"bras_{side}", [(0.22, y, 0.56), (0.25, y, 0.38), (0.24, y, 0.22)],
                 [0.08, 0.062, 0.05], _dark(), sides=7)
    lower = tube(f"avant_bras_{side}", [(0.24, y, 0.22), (0.25, y, 0.04)], [0.048, 0.045],
                 _tan(), sides=7)
    paw = blob(f"pied_av_{side}", 0.06, _tan(), subdivisions=1, scale=(1.3, 1.0, 0.6),
               location=(0.27, y, 0.036))
    return [upper, lower, paw]


def _hind_leg(side):
    """Patte arrière : cuisse épaisse → genou → jarret plié vers l'arrière → pied."""
    y = side * 0.11
    thigh = tube(f"cuisse_{side}", [(-0.26, y, 0.6), (-0.17, y, 0.4), (-0.27, y, 0.24)],
                 [0.11, 0.07, 0.05], _dark(), sides=7)
    shin = tube(f"jarret_{side}", [(-0.27, y, 0.24), (-0.26, y, 0.04)], [0.046, 0.044],
                _tan(), sides=7)
    paw = blob(f"pied_ar_{side}", 0.06, _tan(), subdivisions=1, scale=(1.3, 1.0, 0.6),
               location=(-0.24, y, 0.036))
    return [thigh, shin, paw]


def _tail():
    points = [(-0.34, 0, 0.62), (-0.48, 0, 0.6), (-0.6, 0, 0.52), (-0.68, 0, 0.46)]
    return [tube("queue", points, [0.05, 0.045, 0.032, 0.012], _dark(), sides=6)]


def build():
    parts = _body() + _neck_and_head() + _tail()
    for build_side in (_ear, _face_marks, _front_leg, _hind_leg):
        for group in mirrored(build_side):
            parts += group
    return parts
