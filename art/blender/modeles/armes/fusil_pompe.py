"""Fusil à pompe : l'arme la plus puissante qu'on trouve dans les ruines, rare et bruyante.

Référence : art/concepts/ambiances/zone-4-site-des-portes-lieux.jpeg (La Grande Porte) :
crosse et devant en bois, boîtier gris foncé rouillé, grande hausse sur le dessus.
Ajouts de design : pompe en bois rainurée sur un tube magasin, collier qui relie canon et magasin,
fenêtre d'éjection, ruban adhésif autour du poignet (réparation de fortune), plaque de couche.

Pointé vers +x, debout (comme l'arbalète), pivot au pied. Longueur 0,98 m.
"""
from lowpoly.gun_parts import (barrel, gun_material, metal_piece, segmented, sight, trigger_group,
                               weathered, wood_piece)
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.shapes import box

ICON = True
VIEW = (0.1, -1.0, 0.45)

BORE_Z = 0.124
MAGAZINE_Z = 0.092
TAPE = (0.36, 0.37, 0.36)       # ruban adhésif gris
TAPE_DARK = (0.24, 0.25, 0.25)

# Crosse vue de côté : talon qui plonge, poignet fin, jusqu'au boîtier.
STOCK = [(0.0, 0.012), (0.0, 0.112), (0.05, 0.11), (0.25, 0.128), (0.33, 0.138), (0.33, 0.08),
         (0.29, 0.08), (0.25, 0.07), (0.1, 0.03), (0.02, 0.008)]
RECEIVER = [(0.325, 0.143), (0.52, 0.143), (0.52, 0.08), (0.325, 0.077)]


def _stock():
    """Crosse en bois, plaque de couche, ruban adhésif autour du poignet."""
    parts = wood_piece("crosse", STOCK, 0.04,
                       grain=[(0.04, 0.2, 0.07), (0.07, 0.18, 0.045), (0.1, 0.24, 0.095)], seed=1)
    parts.append(box("plaque_couche", (0.012, 0.042, 0.106), gun_material("bronze_sombre"),
                     location=(-0.005, 0, 0.06)))
    for index, (x, tilt) in enumerate(((0.27, 10), (0.292, -8))):
        tape = box(f"ruban_{index}", (0.02, 0.046, 0.066), flat_material("ruban", TAPE),
                   location=(x, 0, 0.1), rotation_deg=(0, tilt + 25, 0))
        parts.append(two_tone(tape, TAPE, TAPE_DARK, "ruban", threshold=0.3, seed=index))
    return parts


def _receiver():
    """Boîtier en métal, fenêtre d'éjection, grande hausse, pontet."""
    parts = [metal_piece("boitier", RECEIVER, 0.042, wear=4, seed=2)]
    parts.append(box("ejection", (0.06, 0.002, 0.018), gun_material("bouche"),
                     location=(0.44, -0.0215, 0.122)))
    parts.append(sight("hausse", 0.42, 0.143, size=(0.03, 0.012, 0.022)))
    parts += trigger_group("detente", 0.39, 0.079, 0.06, 0.032)
    return parts


def _barrel_and_pump():
    """Canon, tube magasin, pompe en bois rainurée, collier avant et guidon."""
    parts = barrel("canon", 0.5, 0.98, BORE_Z, 0.0155, seed=3)
    points = segmented(0.5, 0.88, 0, MAGAZINE_Z)
    magazine = tube("magasin", points, [0.0125] * len(points), gun_material("bronze"), sides=8)
    parts.append(weathered(magazine, wear=2, seed=4))
    parts += _pump()
    parts.append(weathered(box("collier", (0.018, 0.034, 0.06), gun_material("bronze"),
                               location=(0.86, 0, (BORE_Z + MAGAZINE_Z) / 2)), wear=1, seed=5))
    parts.append(sight("guidon", 0.97, BORE_Z + 0.0145, size=(0.006, 0.005, 0.006)))
    return parts


def _pump():
    """Pompe en bois autour du magasin, avec ses rainures de prise."""
    parts = wood_piece("pompe", [(0.58, 0.114), (0.76, 0.114), (0.76, 0.07), (0.58, 0.07)], 0.05,
                       seed=6)
    groove = flat_material("pompe_rainure", (0.26, 0.17, 0.11))
    for index in range(6):
        x = 0.6 + index * 0.028
        for side in (-1, 1):
            parts.append(box(f"rainure_{index}_{side}", (0.006, 0.002, 0.03), groove,
                             location=(x, side * 0.0252, 0.092)))
    return parts


def build():
    parts = _stock() + _receiver() + _barrel_and_pump()
    return parts
