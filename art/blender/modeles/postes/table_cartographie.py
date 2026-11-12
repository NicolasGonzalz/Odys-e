"""Table de cartographie (poste) : on y assemble la grande carte à partir des fragments trouvés dans
chaque biome, et on y reporte ce qu'on a exploré (docs/GAMEPLAY.md § 3). Fabriquée à l'établi.

Pas de concept dédié : le mobilier de l'établi (lowpoly/carpentry.py). Sur le plateau, une grande
feuille de fond où sont épinglés trois fragments sur cinq, teintés selon leur biome (plaine,
ville, marais) ; les deux emplacements encore vides (montagne, forêt) sont tracés en pointillés.
Lampe à huile allumée, boussole, encrier et plume ; cartes roulées dans un tonnelet.
1,3 × 0,8 m, plateau à 0,85 m.
"""
import math
import random

from lowpoly.carpentry import board, stain
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.outline import without_outline
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.shapes import box, cone, cylinder
from lowpoly.tool_parts import TOOL_COLORS, material

ICON = True
VIEW = (-0.35, -1.0, 0.85)   # de haut : on doit voir la carte

TOP = 0.85
LEG_X, LEG_Y = 0.55, 0.32
PAPER = (0.90, 0.85, 0.72)
INK = (0.30, 0.22, 0.16)
PIN = (0.72, 0.16, 0.12)
FLAME = (1.0, 0.70, 0.30)

# Fragments de la grande carte, un par biome : (x, y, couleur ou None si encore à trouver).
# Disposés comme les zones du monde (docs/ZONES.md) : plaine au sud, ville au centre,
# marais à l'ouest, montagne à l'est, forêt du Cœur au nord.
FRAGMENTS = [("plaine", 0.0, -0.17, (0.62, 0.74, 0.45)),
             ("ville", 0.0, 0.0, (0.66, 0.67, 0.66)),
             ("marais", -0.24, 0.05, (0.62, 0.52, 0.70)),
             ("montagne", 0.24, 0.05, None),
             ("foret", 0.0, 0.19, None)]
FRAGMENT = (0.22, 0.16)


def _table():
    parts = []
    for i, (y, tint) in enumerate(((-0.27, 1.0), (0.0, 0.94), (0.27, 1.05))):
        plank = board(f"plateau_{i}", 1.3, 0.27, 0.045, tint=tint, seed=40 + i, grain=3)
        parts += posed(plank, (0, 0, 0), (0, y, TOP - 0.0225))
    for sx in (-1, 1):
        for sy in (-1, 1):
            leg = board(f"pied_{sx}_{sy}", TOP - 0.045, 0.07, 0.07, ("poutre", "poutre_sombre"),
                        seed=45 + sx + 2 * sy, grain=1)
            parts += posed(leg, (0, 90, 0), (sx * LEG_X, sy * LEG_Y, (TOP - 0.045) / 2))
    for sy in (-1, 1):
        beam = board(f"traverse_{sy}", 2 * LEG_X + 0.06, 0.08, 0.035, ("poutre", "poutre_sombre"),
                     seed=50 + sy, grain=0)
        parts += posed(beam, (90, 0, 0), (0, sy * (LEG_Y + 0.055), 0.18))
    return parts


def _paper(name, center, size, color, angle, z):
    """Une feuille plate (légèrement de travers), coins un peu irréguliers."""
    rng = random.Random(len(name))
    w, h = size[0] / 2, size[1] / 2
    corners = [(-w + rng.uniform(0, 0.008), -h), (w, -h + rng.uniform(0, 0.008)),
               (w - rng.uniform(0, 0.008), h), (-w, h - rng.uniform(0, 0.008))]
    sheet = profile_solid(name, corners, lambda _: 0.0015, [flat_material(f"{name}_papier", color)],
                          lambda _: 0, chamfer=0.0004)
    two_tone(sheet, color, tuple(c * 0.9 for c in color), f"{name}_papier", threshold=0.5)
    return posed([sheet], (90, 0, angle), (center[0], center[1], z))


def _map():
    """Feuille de fond, fragments épinglés, emplacements vides en pointillés."""
    parts = _paper("carte_fond", (0.0, 0.0), (0.78, 0.56), PAPER, -3, TOP + 0.001)
    for index, (biome, x, y, color) in enumerate(FRAGMENTS):
        if color:
            parts += _paper(f"fragment_{biome}", (x, y), FRAGMENT, color, (index * 7) % 9 - 4,
                            TOP + 0.003)
            parts += _pin(f"epingle_{biome}", x - 0.08, y + 0.05)
            parts.append(stain(f"trace_{biome}", (x + 0.02, y - 0.01, TOP + 0.0046), 0.03,
                               tuple(c * 0.7 for c in color), seed=index))
        else:
            parts += _dashed_slot(biome, x, y)
    return parts


def _pin(name, x, y):
    """Épingle à tête rouge."""
    return [cylinder(f"{name}_tige", 0.0015, 0.02, material("metal_clair"), vertices=4,
                     location=(x, y, TOP + 0.012)),
            cylinder(f"{name}_tete", 0.006, 0.008, flat_material("epingle", PIN), vertices=6,
                     location=(x, y, TOP + 0.024))]


def _dashed_slot(name, x, y):
    """Emplacement d'un fragment pas encore trouvé : un cadre en pointillés."""
    w, h = FRAGMENT[0] / 2, FRAGMENT[1] / 2
    corners = [(x - w, y - h), (x + w, y - h), (x + w, y + h), (x - w, y + h)]
    ink = flat_material("carte_encre", INK)
    dashes = []
    for side, (a, b) in enumerate(zip(corners, corners[1:] + corners[:1])):
        for k in range(0, 6, 2):
            p0 = (a[0] + (b[0] - a[0]) * k / 6, a[1] + (b[1] - a[1]) * k / 6, TOP + 0.003)
            p1 = (a[0] + (b[0] - a[0]) * (k + 1) / 6, a[1] + (b[1] - a[1]) * (k + 1) / 6, TOP + 0.003)
            dashes.append(without_outline(tube(f"vide_{name}_{side}_{k}", [p0, p1], [0.0018] * 2, ink,
                                               sides=3)))
    return dashes


def _lamp():
    """Lampe à huile allumée, au coin arrière gauche : la flamme se voit entre les fils de sa cage
    (un verre opaque la cacherait)."""
    x, y, z = -0.5, 0.25, TOP
    brass = flat_material("lampe_laiton", (0.70, 0.55, 0.28))
    parts = [cylinder("lampe_pied", 0.05, 0.035, brass, vertices=8, location=(x, y, z + 0.0175)),
             cone("lampe_flamme", 0.014, 0.05, flat_material("lampe_flamme", FLAME, emission=4.0),
                  vertices=6, location=(x, y, z + 0.06)),
             cylinder("lampe_chapeau", 0.032, 0.02, brass, vertices=8, location=(x, y, z + 0.15))]
    for i in range(4):
        angle = math.tau * i / 4 + 0.4
        px, py = x + math.cos(angle) * 0.03, y + math.sin(angle) * 0.03
        parts.append(tube(f"lampe_cage_{i}", [(px, py, z + 0.035), (px, py, z + 0.14)], [0.003] * 2,
                          brass, sides=4))
    return parts


def _desk_items():
    """Boussole, encrier et plume, crayon."""
    x, y, z = 0.45, -0.22, TOP
    parts = [cylinder("boussole", 0.04, 0.012, flat_material("boussole", (0.70, 0.55, 0.28)),
                      vertices=10, location=(x, y, z + 0.006)),
             cylinder("boussole_cadran", 0.033, 0.002, flat_material("cadran", PAPER), vertices=10,
                      location=(x, y, z + 0.013)),
             box("boussole_aiguille", (0.05, 0.006, 0.002), flat_material("aiguille", PIN),
                 location=(x, y, z + 0.015), rotation_deg=(0, 0, 30))]
    ink_x, ink_y = 0.48, 0.2
    parts.append(cylinder("encrier", 0.03, 0.05, flat_material("encrier", (0.15, 0.17, 0.2)),
                          vertices=8, location=(ink_x, ink_y, z + 0.025)))
    parts.append(tube("plume", [(ink_x, ink_y, z + 0.04), (ink_x - 0.06, ink_y + 0.04, z + 0.17)],
                      [0.004, 0.012], flat_material("plume", (0.88, 0.86, 0.80)), sides=4))
    return parts


def _rolled_maps():
    """Tonnelet de cartes roulées, au pied de la table, côté droit."""
    x, y = 0.82, 0.2
    barrel = cylinder("tonnelet", 0.15, 0.4, material("bois"), vertices=10, location=(x, y, 0.2),
                      radius_top=0.14)
    two_tone(barrel, (0.55, 0.39, 0.26), (0.40, 0.28, 0.19), "tonnelet", seed=60)
    parts = [barrel]
    for i, height in enumerate((0.25, 0.32, 0.20)):
        rolls = cylinder(f"carte_roulee_{i}", 0.025, 0.4, flat_material("roulee", PAPER), vertices=6,
                         location=(x + (i - 1) * 0.05, y + (i % 2) * 0.04, 0.4 + height / 2 - 0.05),
                         rotation_deg=((i - 1) * 8, (i - 1) * 6, 0))
        parts.append(rolls)
    for i, z in enumerate((0.08, 0.32)):
        parts.append(cylinder(f"tonnelet_cercle_{i}", 0.152 - z * 0.03, 0.025,
                              material("metal_sombre"), vertices=10, location=(x, y, z)))
    return parts


def build():
    return _table() + _map() + _lamp() + _desk_items() + _rolled_maps()
