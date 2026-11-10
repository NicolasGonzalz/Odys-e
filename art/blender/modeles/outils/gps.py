"""GPS / détecteur : carte de la zone, et bip quand un cocon ou un artefact est proche.

Références : art/concepts/ambiances/zone-4-site-des-portes-lieux.jpeg (Flore & Gaïa) et
zones-1-3-5-lieux.jpeg (Le Sanctuaire) : boîtier durci gris, écran vert affichant une carte.
Ajouts : coins en caoutchouc, antenne du détecteur, voyant vert, trois boutons.

Couché écran vers le haut (+z), antenne vers +y. 0,12 × 0,18 m. L'écran est émissif.
"""
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.shapes import bevel, box, cylinder

ICON = True
VIEW = (0.3, -0.6, 1.0)   # de haut : on voit l'écran

CASE = (0.45, 0.46, 0.46)
CASE_DARK = (0.30, 0.31, 0.32)
RUBBER = (0.16, 0.17, 0.17)
SCREEN = (0.06, 0.22, 0.11)
MAP = (0.45, 0.95, 0.45)
WIDTH, HEIGHT, DEPTH = 0.11, 0.16, 0.02
TOP = DEPTH

# Tracés de la carte sur l'écran (x, y), en mètres depuis le centre de l'écran.
MAP_LINES = [
    [(-0.035, 0.03), (-0.015, 0.045), (0.01, 0.035), (0.03, 0.045)],                 # rivière
    [(-0.02, -0.01), (0.0, 0.005), (0.022, -0.004), (0.015, -0.028), (-0.012, -0.03),
     (-0.02, -0.01)],                                                               # zone
    [(-0.04, -0.045), (-0.01, -0.02), (0.035, 0.01)],                                # chemin
]


def _case():
    """Boîtier durci, deux tons, coins en caoutchouc."""
    case = box("boitier", (WIDTH, HEIGHT, DEPTH), flat_material("gps_boitier", CASE),
               location=(0, 0, DEPTH / 2))
    bevel(case, 0.004)
    two_tone(case, CASE, CASE_DARK, "gps_boitier", dark_side=(0.3, 0.3, -1), seed=1)
    rubber = flat_material("gps_caoutchouc", RUBBER)
    corners = [box(f"coin_{sx}_{sy}", (0.022, 0.022, DEPTH + 0.004), rubber,
                   location=(sx * (WIDTH / 2 - 0.008), sy * (HEIGHT / 2 - 0.008), DEPTH / 2))
               for sx in (-1, 1) for sy in (-1, 1)]
    return [case, *corners]


def _screen():
    """Écran vert sombre et lumineux, avec la carte tracée en vert vif."""
    center_y = 0.018
    parts = [box("ecran", (0.088, 0.1, 0.002), flat_material("gps_ecran", SCREEN, emission=1.0),
                 location=(0, center_y, TOP + 0.0005))]
    glow = flat_material("gps_carte", MAP, emission=2.5)
    for index, line in enumerate(MAP_LINES):
        points = [(x, center_y + y, TOP + 0.0018) for x, y in line]
        parts.append(tube(f"carte_{index}", points, [0.0011] * len(points), glow, sides=3))
    parts.append(cylinder("position", 0.003, 0.001, glow, vertices=6,
                          location=(0.006, center_y + 0.008, TOP + 0.002)))
    return parts


def _controls():
    """Trois boutons sous l'écran, voyant vert, antenne du détecteur."""
    dark = flat_material("gps_bouton", RUBBER)
    parts = [cylinder(f"bouton_{i}", 0.006, 0.004, dark, vertices=8,
                      location=(x, -0.055, TOP + 0.001)) for i, x in enumerate((-0.025, 0, 0.025))]
    parts.append(cylinder("voyant", 0.0025, 0.003, flat_material("gps_voyant", MAP, emission=3.0),
                          vertices=6, location=(0.04, 0.072, TOP + 0.001)))
    parts.append(cylinder("antenne", 0.005, 0.05, dark, vertices=6,
                          location=(-0.035, HEIGHT / 2 + 0.022, DEPTH / 2), rotation_deg=(90, 0, 0)))
    parts.append(cylinder("antenne_bout", 0.007, 0.008, dark, vertices=6,
                          location=(-0.035, HEIGHT / 2 + 0.048, DEPTH / 2), rotation_deg=(90, 0, 0)))
    return parts


def build():
    return _case() + _screen() + _controls()
