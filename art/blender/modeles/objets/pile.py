"""Pile : l'énergie de la lampe torche et du GPS. On la fouille dans les ruines (voitures,
boîtes à outils, tiroirs) ; elle s'use, il faut en retrouver.

Pas de concept dédié : une grosse pile ronde classique, étiquette délavée, bouts rouillés, pour
qu'on la reconnaisse en icône. Couchée, borne + vers +x. Longueur 6 cm.
"""
from lowpoly.materials import flat_material
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import cylinder
from lowpoly.tool_parts import material

ICON = True
VIEW = (0.4, -1.0, 0.6)

RADIUS = 0.017
LENGTH = 0.058
SIDES = 10
LABEL = (0.80, 0.64, 0.28)        # jaune délavé
LABEL_DARK = (0.58, 0.44, 0.18)
BAND = (0.16, 0.16, 0.17)         # bande sombre de l'étiquette
CASE = (0.62, 0.62, 0.6)          # bouts en métal


def _along_x(name, radius, depth, material_, x):
    """Cylindre couché le long de x, centré en x."""
    return cylinder(name, radius, depth, material_, vertices=SIDES, location=(x, 0, RADIUS),
                    rotation_deg=(0, 90, 0))


def build():
    body = _along_x("pile", RADIUS, LENGTH * 0.86, flat_material("pile_etiquette", LABEL), 0.0)
    two_tone(body, LABEL, LABEL_DARK, "pile_etiquette", dark_side=(0, 0.3, -1), seed=1)
    blotches(body, flat_material("pile_usure", (0.88, 0.82, 0.66)), count=2, radius=0.012, seed=2)
    band = _along_x("pile_bande", RADIUS + 0.0004, LENGTH * 0.22, flat_material("pile_bande", BAND),
                    -0.012)
    ends = [_along_x(f"pile_bout_{i}", RADIUS * 0.97, LENGTH * 0.08,
                     flat_material("pile_metal", CASE), side * LENGTH * 0.45)
            for i, side in enumerate((-1, 1))]
    for index, end in enumerate(ends):
        blotches(end, material("rouille"), count=2, radius=0.01, seed=index + 4)
    terminal = _along_x("pile_borne", RADIUS * 0.32, 0.006, material("metal_clair"),
                        LENGTH * 0.5 + 0.002)
    return [body, band, *ends, terminal]
