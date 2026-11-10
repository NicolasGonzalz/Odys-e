"""Loup (faune revenue) low-poly : corps, tête à museau, oreilles, pattes, queue.

Référence : art/concepts/creatures/bestiaire.jpg (faune revenue).
Tourné vers +x. Hauteur au garrot ~0,8 m.
"""
from lowpoly.materials import flat_material
from lowpoly.shapes import blob, cone, cylinder, roughen, wedge

FUR = (0.42, 0.40, 0.37)
FUR_LIGHT = (0.68, 0.64, 0.58)
FUR_DARK = (0.24, 0.22, 0.21)


def _body():
    fur = flat_material("pelage", FUR)
    belly = flat_material("pelage_clair", FUR_LIGHT)
    torso = blob("torse", 0.5, fur, subdivisions=1, scale=(1.0, 0.42, 0.45), location=(0, 0, 0.72))
    chest = blob("poitrail", 0.32, belly, subdivisions=1, scale=(0.9, 0.85, 1.0),
                 location=(0.36, 0, 0.66))
    neck = cylinder("cou", 0.17, 0.32, fur, vertices=6, location=(0.5, 0, 0.88),
                    rotation_deg=(0, -50, 0))
    return [roughen(torso, 0.025, 1), roughen(chest, 0.02, 2), neck]


def _head():
    fur = flat_material("pelage", FUR)
    dark = flat_material("pelage_sombre", FUR_DARK)
    skull = blob("crane", 0.17, fur, subdivisions=1, scale=(1.1, 0.9, 0.85),
                 location=(0.66, 0, 1.0))
    muzzle = wedge("museau", 0.24, back=(0.17, 0.15), front=(0.07, 0.07), material=fur,
                   location=(0.74, 0, 0.96))
    nose = blob("truffe", 0.035, dark, subdivisions=1, location=(0.99, 0, 0.97))
    ears = [cone(f"oreille_{side}", 0.06, 0.15, dark, vertices=4,
                 location=(0.62, side * 0.08, 1.15), rotation_deg=(side * -12, -10, 0))
            for side in (-1, 1)]
    return [skull, muzzle, nose, *ears]


def _legs():
    fur = flat_material("pelage", FUR)
    dark = flat_material("pelage_sombre", FUR_DARK)
    legs = []
    for x in (0.32, -0.32):
        for side in (-1, 1):
            legs.append(cylinder(f"patte_{x}_{side}", 0.06, 0.62, fur, vertices=5,
                                 location=(x, side * 0.13, 0.31), radius_top=0.075))
            legs.append(blob(f"pied_{x}_{side}", 0.055, dark, subdivisions=1,
                             scale=(1.4, 1.0, 0.6), location=(x + 0.03, side * 0.13, 0.03)))
    return legs


def _tail():
    fur = flat_material("pelage", FUR)
    tail = cone("queue", 0.09, 0.55, fur, vertices=5, location=(-0.68, 0, 0.6),
                rotation_deg=(0, -125, 0))
    return [roughen(tail, 0.015, 3)]


def build():
    return _body() + _head() + _legs() + _tail()
