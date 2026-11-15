"""Viande de gibier (ressource, grosse viande) : obtenue en dépeçant un cerf ou un grand animal.

Sans concept : un gros cuissot cru, chair rouge sombre marbrée de gras crème, os épais qui
dépasse avec sa tête articulaire. ~30 cm, deux fois la viande d'oiseau, pour qu'on ne les
confonde pas. Choix de design à valider.
"""
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.rock import on_surface, rock
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import blob, cylinder

ICON = True
VIEW = (0.4, -1.0, 0.75)

MEAT = (0.66, 0.18, 0.16)        # chair rouge sombre
MEAT_DARK = (0.44, 0.10, 0.10)
FAT = (0.94, 0.87, 0.76)         # gras crème
BONE = (0.92, 0.87, 0.76)
MARROW = (0.62, 0.36, 0.28)


def _haunch():
    """Masse charnue : un bloc irrégulier, plus large côté cuisse (x < 0)."""
    meat = rock("cuissot", (0.15, 0.1, 0.06), flat_material("gibier_chair", MEAT), count=30,
                dents=2, base=0.8, facets=0.06, seed=14)
    two_tone(meat, MEAT, MEAT_DARK, "gibier_chair", dark_side=(0.3, 0.7, -0.6), threshold=0.0,
             spread=0.07, seed=14)
    blotches(meat, flat_material("gibier_gras", FAT), count=9, radius=0.016, seed=3)
    return meat


def _bone(meat):
    """Os qui sort du bout étroit, et sa coupe (moelle) sur le dessus du côté large."""
    bone = flat_material("gibier_os", BONE)
    shaft = tube("os", [(0.08, 0, 0.05), (0.15, 0, 0.055), (0.2, 0, 0.058)],
                 [0.017, 0.015, 0.016], bone, sides=7)
    head = [blob(f"tete_os_{i}", 0.024, bone, location=(0.215, y, 0.058)) for i, y in
            enumerate((-0.013, 0.013))]
    top = on_surface(meat, -0.06, 0.0)
    cut = cylinder("coupe_os", 0.02, 0.012, bone, vertices=9, location=top)
    marrow = cylinder("moelle", 0.011, 0.014, flat_material("gibier_moelle", MARROW), vertices=7,
                      location=top)
    return [shaft, *head, cut, marrow]


def build():
    meat = _haunch()
    return [meat, *_bone(meat)]
