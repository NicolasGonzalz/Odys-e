"""Viande d'oiseau (ressource, petite viande) : tombe des oiseaux abattus.

Sans concept : une cuisse crue, chair rose pâle et peau plus claire, os qui dépasse avec sa
rotule. Petite (~15 cm) : elle doit se distinguer de la viande de gibier, grosse et rouge sombre.
Choix de design à valider.
"""
from lowpoly.materials import flat_material
from lowpoly.organic import loft, tube
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import blob, roughen

ICON = True
VIEW = (0.35, -1.0, 0.8)

MEAT = (0.90, 0.60, 0.55)        # chair crue, rose pâle
MEAT_DARK = (0.70, 0.40, 0.37)
SKIN = (0.96, 0.84, 0.72)        # peau de volaille
BONE = (0.93, 0.89, 0.79)

# Tranches de la cuisse (x, z_centre, demi-largeur, demi-hauteur), du genou à la partie charnue.
SECTIONS = [(0.0, 0.02, 0.011, 0.011), (0.025, 0.024, 0.022, 0.02), (0.06, 0.028, 0.033, 0.029),
            (0.09, 0.028, 0.034, 0.03), (0.115, 0.025, 0.024, 0.022), (0.128, 0.022, 0.01, 0.01)]


def _meat():
    thigh = loft("cuisse", SECTIONS, flat_material("viande_oiseau_chair", MEAT), sides=8)
    roughen(thigh, 0.002, seed=3)
    two_tone(thigh, MEAT, MEAT_DARK, "viande_oiseau_chair", dark_side=(0.2, 0.6, -0.8),
             threshold=0.05, spread=0.05, seed=3)
    blotches(thigh, flat_material("viande_oiseau_peau", SKIN), count=3, radius=0.02, seed=5)
    return thigh


def _bone():
    bone = flat_material("viande_oiseau_os", BONE)
    shaft = tube("os", [(-0.05, 0, 0.017), (-0.02, 0, 0.018), (0.01, 0, 0.02)],
                 [0.0065, 0.006, 0.007], bone, sides=6)
    knob = [blob(f"rotule_{i}", 0.0105, bone, location=(-0.056, y, 0.017)) for i, y in
            enumerate((-0.006, 0.006))]
    return [shaft, *knob]


def build():
    return [_meat(), *_bone()]
