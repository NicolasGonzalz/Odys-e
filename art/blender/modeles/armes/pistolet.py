"""Pistolet : vieux revolver à canon long, trouvé dans les ruines.

Référence : art/concepts/ambiances/zones-1-a-5-planche-a.jpeg (Le Site des Portes, le hangar) :
canon long gris foncé, carcasse anguleuse, crosse en bois rouge-brun, devant en bois sous le canon.
Ajouts de design : barillet à six pans (on le lit tout de suite comme une arme à six coups), chien,
guidon, vis de crosse, usure et rouille.

Pointé vers +x, debout (comme l'arbalète), pivot au pied. Longueur 0,30 m.
"""
from lowpoly.gun_parts import (barrel, gun_material, metal_piece, sight, trigger_group, weathered,
                               wood_piece)
from lowpoly.shapes import cylinder, wedge

ICON = True
VIEW = (0.15, -1.0, 0.25)

BORE_Z = 0.105   # hauteur de l'axe du canon

FRAME = [(-0.012, 0.118), (0.075, 0.12), (0.075, 0.088), (0.04, 0.074), (0.0, 0.074), (-0.018, 0.09)]
GRIP = [(-0.016, 0.092), (0.012, 0.076), (-0.012, 0.0), (-0.054, 0.008), (-0.048, 0.035), (-0.032, 0.07)]
FOREND = [(0.08, 0.094), (0.2, 0.094), (0.2, 0.084), (0.09, 0.08)]


def _frame():
    """Carcasse en métal et barillet à six pans."""
    parts = [metal_piece("carcasse", FRAME, 0.024, wear=2, seed=1)]
    drum = cylinder("barillet", 0.021, 0.044, gun_material("bronze"), vertices=6,
                    location=(0.04, 0, BORE_Z - 0.006), rotation_deg=(0, 90, 0))
    parts.append(weathered(drum, wear=2, seed=2))
    parts.append(cylinder("barillet_axe", 0.006, 0.05, gun_material("bronze_sombre"), vertices=6,
                          location=(0.04, 0, BORE_Z - 0.006), rotation_deg=(0, 90, 0)))
    parts.append(wedge("chien", 0.024, (0.008, 0.014), (0.006, 0.005), gun_material("bronze_sombre"),
                       location=(-0.004, 0, 0.116), rotation_deg=(0, -50, 0)))
    return parts


def _grip():
    """Crosse en bois, une vis de chaque côté, calotte en métal."""
    parts = wood_piece("crosse", GRIP, 0.03, grain=[(-0.04, -0.008, 0.03)], seed=3)
    for side in (-1, 1):
        parts.append(cylinder(f"vis_{side}", 0.004, 0.003, gun_material("usure"), vertices=6,
                              location=(-0.026, side * 0.0155, 0.045), rotation_deg=(90, 0, 0)))
    parts.append(metal_piece("calotte", [(-0.055, 0.004), (-0.011, -0.004), (-0.009, 0.006),
                                         (-0.053, 0.013)], 0.032, wear=1, seed=4))
    return parts


def _barrel():
    """Canon long, devant en bois dessous, guidon au bout."""
    parts = barrel("canon", 0.06, 0.29, BORE_Z, 0.0105, seed=5)
    parts += wood_piece("devant", FOREND, 0.02, seed=6)
    parts.append(sight("guidon", 0.283, BORE_Z + 0.009))
    return parts


def build():
    parts = _frame() + _grip() + _barrel() + trigger_group("detente", 0.04, 0.075, 0.045, 0.028)
    return parts
