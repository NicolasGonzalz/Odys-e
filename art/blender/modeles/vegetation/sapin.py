"""Sapin low-poly : tronc court et étages de cônes, du plus large au plus petit.

Référence : art/concepts/decors/vegetation-ruines-campement.jpg (le conifère).
Hauteur ~7 m.
"""
from lowpoly.materials import flat_material
from lowpoly.shapes import cone, cylinder, roughen

TIERS = [  # (rayon, hauteur, altitude du centre) de chaque étage de branches, en mètres
    (2.0, 2.6, 2.4),
    (1.6, 2.2, 3.6),
    (1.15, 1.9, 4.7),
    (0.7, 1.6, 5.7),
]


def build():
    trunk = cylinder("tronc", 0.22, 2.0, flat_material("bois_sombre"), vertices=6,
                     location=(0, 0, 1.0), radius_top=0.16)
    foliage = flat_material("feuillage_sombre")
    tiers = []
    for index, (radius, height, z) in enumerate(TIERS):
        tier = cone(f"etage_{index}", radius, height, foliage, vertices=7, location=(0, 0, z),
                    rotation_deg=(0, 0, index * 23))
        tiers.append(roughen(tier, 0.06, seed=index))
    return [trunk, *tiers]
