"""Table de cartographie (poste) : on y assemble la grande carte à partir des fragments trouvés dans
chaque biome, et on y reporte ce qu'on a exploré (docs/GAMEPLAY.md § 3). Fabriquée à l'établi.

Pas de concept dédié. Pour ne pas la confondre avec l'établi (table plate à quatre pieds), c'est un
pupitre de cartographe : plateau incliné vers le joueur, rebord en bas pour retenir crayons et
boussole, pieds en X comme des tréteaux, poteau avec une lanterne suspendue (silhouette haute,
reconnaissable de loin), cartes roulées dans un tonnelet.
Sur le plateau : une grande feuille où sont épinglés trois fragments sur cinq, teintés selon leur
biome (plaine, ville, marais) ; les deux emplacements encore vides (montagne, forêt) en pointillés.
1,3 × 0,8 m, 2 m de haut avec la lanterne.
"""
import math
import random

from lowpoly.carpentry import board, stain
from lowpoly.finish import posed
from lowpoly.fire import AMBER, flame
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.outline import without_outline
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.shapes import box, cylinder
from lowpoly.tool_parts import material

ICON = True
VIEW = (-0.45, -1.0, 0.7)

BOARD_HEIGHT = 0.92    # centre du plateau
TILT = 20              # inclinaison du plateau vers le joueur (degrés)
BOARD = (1.2, 0.78)    # largeur, profondeur du plateau
LEG_X = 0.5            # position des deux tréteaux
POST = (-0.66, 0.36)   # pied du poteau de la lanterne
PAPER = (0.90, 0.85, 0.72)
INK = (0.30, 0.22, 0.16)
PIN = (0.72, 0.16, 0.12)
BRASS = (0.70, 0.55, 0.28)

# Fragments de la grande carte, un par biome : (biome, x, y, couleur ou None si encore à trouver).
# Disposés comme les zones du monde (docs/ZONES.md) : plaine au sud, ville au centre,
# marais à l'ouest, montagne à l'est, forêt du Cœur au nord.
FRAGMENTS = [("plaine", 0.0, -0.17, (0.62, 0.74, 0.45)),
             ("ville", 0.0, 0.0, (0.66, 0.67, 0.66)),
             ("marais", -0.24, 0.05, (0.62, 0.52, 0.70)),
             ("montagne", 0.24, 0.05, None),
             ("foret", 0.0, 0.19, None)]
FRAGMENT = (0.22, 0.16)


# --- Plateau (construit à plat, dessus à z = 0, puis incliné) ------------------------------------

def _board_planks():
    parts = []
    for i, (y, tint) in enumerate(((-0.26, 1.0), (0.0, 0.94), (0.26, 1.05))):
        plank = board(f"plateau_{i}", BOARD[0], 0.26, 0.04, tint=tint, seed=40 + i, grain=3)
        parts += posed(plank, (0, 0, 0), (0, y, -0.02))
    lip = board("rebord", BOARD[0], 0.05, 0.03, ("poutre", "poutre_sombre"), seed=44, grain=0)
    parts += posed(lip, (90, 0, 0), (0, -BOARD[1] / 2 - 0.01, 0.0))  # retient crayons et boussole
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
    parts = _paper("carte_fond", (0.0, 0.02), (0.78, 0.56), PAPER, -2, 0.001)
    for index, (biome, x, y, color) in enumerate(FRAGMENTS):
        if color:
            parts += _paper(f"fragment_{biome}", (x, y), FRAGMENT, color, (index * 7) % 9 - 4, 0.003)
            parts += _pin(f"epingle_{biome}", x - 0.08, y + 0.05)
            parts.append(stain(f"trace_{biome}", (x + 0.02, y - 0.01, 0.0046), 0.03,
                               tuple(c * 0.7 for c in color), seed=index))
        else:
            parts += _dashed_slot(biome, x, y)
    return parts


def _pin(name, x, y):
    """Épingle à tête rouge."""
    return [cylinder(f"{name}_tige", 0.0015, 0.02, material("metal_clair"), vertices=4,
                     location=(x, y, 0.012)),
            cylinder(f"{name}_tete", 0.006, 0.008, flat_material("epingle", PIN), vertices=6,
                     location=(x, y, 0.024))]


def _dashed_slot(name, x, y):
    """Emplacement d'un fragment pas encore trouvé : un cadre en pointillés."""
    w, h = FRAGMENT[0] / 2, FRAGMENT[1] / 2
    corners = [(x - w, y - h), (x + w, y - h), (x + w, y + h), (x - w, y + h)]
    ink = flat_material("carte_encre", INK)
    dashes = []
    for side, (a, b) in enumerate(zip(corners, corners[1:] + corners[:1])):
        for k in range(0, 6, 2):
            p0 = (a[0] + (b[0] - a[0]) * k / 6, a[1] + (b[1] - a[1]) * k / 6, 0.003)
            p1 = (a[0] + (b[0] - a[0]) * (k + 1) / 6, a[1] + (b[1] - a[1]) * (k + 1) / 6, 0.003)
            dashes.append(without_outline(tube(f"vide_{name}_{side}_{k}", [p0, p1], [0.0018] * 2,
                                               ink, sides=3)))
    return dashes


def _board_items():
    """Boussole et crayon posés contre le rebord, compas de navigation sur la carte."""
    y = -BOARD[1] / 2 + 0.04
    parts = [cylinder("boussole", 0.04, 0.012, flat_material("boussole", BRASS), vertices=10,
                      location=(0.45, y, 0.006)),
             cylinder("boussole_cadran", 0.033, 0.002, flat_material("cadran", PAPER), vertices=10,
                      location=(0.45, y, 0.013)),
             box("boussole_aiguille", (0.05, 0.006, 0.002), flat_material("aiguille", PIN),
                 location=(0.45, y, 0.015), rotation_deg=(0, 0, 30)),
             cylinder("crayon", 0.005, 0.14, flat_material("crayon", (0.45, 0.75, 0.30)),
                      vertices=6, location=(-0.4, y - 0.005, 0.008), rotation_deg=(0, 90, 5))]
    for i, angle in enumerate((-14, 14)):  # compas à pointes sèches, ouvert sur la carte
        a = math.radians(angle)
        parts.append(tube(f"compas_{i}", [(0.3, 0.12, 0.008),
                                          (0.3 + math.sin(a) * 0.12, 0.12 - math.cos(a) * 0.12,
                                           0.004)], [0.003, 0.0015], flat_material("compas", BRASS),
                          sides=4))
    return parts


def _tilted_board():
    flat = _board_planks() + _map() + _board_items()
    return posed(flat, (TILT, 0, 0), (0, 0, BOARD_HEIGHT))


# --- Pieds en X, poteau et lanterne --------------------------------------------------------------

def _beam(name, start, end, size, seed):
    """Poutre de `start` à `end` (points 3D)."""
    dx, dy, dz = (end[i] - start[i] for i in range(3))
    length = math.sqrt(dx * dx + dy * dy + dz * dz)
    pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy)))
    yaw = math.degrees(math.atan2(dy, dx))
    beam = board(name, length, size, size, ("poutre", "poutre_sombre"), seed=seed, grain=0)
    middle = tuple((start[i] + end[i]) / 2 for i in range(3))
    return posed(beam, (0, -pitch, yaw), middle)


def _trestles():
    """Deux tréteaux en X sous le plateau, une traverse qui les relie."""
    parts = []
    top = BOARD_HEIGHT - 0.06
    for i, x in enumerate((-LEG_X, LEG_X)):
        parts += _beam(f"pied_{i}_a", (x, -0.32, 0.0), (x, 0.24, top + 0.08), 0.06, 50 + i)
        parts += _beam(f"pied_{i}_b", (x, 0.32, 0.0), (x, -0.24, top - 0.08), 0.06, 52 + i)
    parts += _beam("traverse", (-LEG_X - 0.04, 0.0, 0.38), (LEG_X + 0.04, 0.0, 0.38), 0.05, 54)
    return parts


def _lantern_post():
    """Poteau à l'arrière gauche, potence, lanterne suspendue allumée."""
    x, y = POST
    parts = _beam("poteau", (x, y, 0.0), (x, y, 2.0), 0.06, 56)
    parts += _beam("potence", (x, y, 1.9), (x + 0.42, y, 1.9), 0.045, 57)
    lx = x + 0.38
    parts.append(tube("lanterne_chaine", [(lx, y, 1.87), (lx, y, 1.66)], [0.004] * 2,
                      material("metal_sombre"), sides=4))
    brass = flat_material("lanterne_laiton", BRASS)
    parts += [cylinder("lanterne_chapeau", 0.07, 0.04, brass, vertices=8, location=(lx, y, 1.64),
                       radius_top=0.03),
              cylinder("lanterne_socle", 0.06, 0.03, brass, vertices=8, location=(lx, y, 1.45))]
    for i in range(4):  # cage : quatre montants, la flamme se voit entre eux
        angle = math.tau * i / 4 + 0.4
        px, py = lx + math.cos(angle) * 0.05, y + math.sin(angle) * 0.05
        parts.append(tube(f"lanterne_montant_{i}", [(px, py, 1.46), (px, py, 1.62)], [0.004] * 2,
                          brass, sides=4))
    return parts + flame("lanterne_flamme", AMBER, 0.08, 0.045, offset=(lx, y, 1.47))


def _rolled_maps():
    """Tonnelet de cartes roulées, au pied du pupitre, côté droit."""
    x, y = 0.82, 0.2
    barrel = cylinder("tonnelet", 0.15, 0.4, material("bois"), vertices=10, location=(x, y, 0.2),
                      radius_top=0.14)
    two_tone(barrel, (0.55, 0.39, 0.26), (0.40, 0.28, 0.19), "tonnelet", seed=60)
    parts = [barrel]
    for i, height in enumerate((0.25, 0.32, 0.20)):
        parts.append(cylinder(f"carte_roulee_{i}", 0.025, 0.4, flat_material("roulee", PAPER),
                              vertices=6, location=(x + (i - 1) * 0.05, y + (i % 2) * 0.04,
                                                    0.4 + height / 2 - 0.05),
                              rotation_deg=((i - 1) * 8, (i - 1) * 6, 0)))
    for i, z in enumerate((0.08, 0.32)):
        parts.append(cylinder(f"tonnelet_cercle_{i}", 0.152 - z * 0.03, 0.025,
                              material("metal_sombre"), vertices=10, location=(x, y, z)))
    return parts


def build():
    return _tilted_board() + _trestles() + _lantern_post() + _rolled_maps()
