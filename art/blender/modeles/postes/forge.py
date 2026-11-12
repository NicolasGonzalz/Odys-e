"""Forge (poste d'artisanat) : on y fond le minerai en lingots, puis on travaille le métal
(docs/GAMEPLAY.md § 2). Fabriquée à l'établi.

Pas de concept dédié : même esprit « récupération » que l'établi. Foyer maçonné en briques de
pierre aux arêtes arrondies (posées en quinconce, margelle lisse) et braises qui rougeoient, creuset de métal en fusion posé dessus, hotte en tôles dépareillées et
rivetées, tuyau de cheminée, soufflet en bois et cuir sur le côté, enclume sur un billot, tas de
charbon. Braises et métal en fusion émettent de la lumière. Environ 1,7 × 1,1 m, 2,3 m de haut.
"""
import math
import random

from lowpoly.carpentry import board
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.profile import profile_solid
from lowpoly.rock import rock
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import bevel, box, cone, cylinder
from lowpoly.tool_parts import TOOL_COLORS, material

ICON = True
VIEW = (-0.6, -1.0, 0.45)

HEARTH = (0.95, 0.75)  # foyer : longueur (x), profondeur (y)
HEARTH_TOP = 0.72               # dessus des briques (six rangs)
HOOD_BOTTOM = 1.25
STONE = (0.56, 0.52, 0.46)       # pierre de champ, grise et chaude
STONE_DARK = (0.38, 0.35, 0.32)
ASH = (0.20, 0.18, 0.17)
EMBER = (1.0, 0.42, 0.12)
EMBER_HOT = (1.0, 0.75, 0.30)
MOLTEN = (1.0, 0.62, 0.18)
LEATHER = (0.40, 0.25, 0.16)
COAL = (0.12, 0.12, 0.13)


# --- Foyer ---------------------------------------------------------------------------------------

BRICK = (0.24, 0.12, 0.115)   # longueur, profondeur, hauteur d'une brique de pierre taillée
COURSES = 6                    # rangs jusqu'à la margelle


def _brick(name, center, along_x, tint):
    """Une brique aux arêtes arrondies, teinte légèrement variée."""
    size = BRICK if along_x else (BRICK[1], BRICK[0], BRICK[2])
    light = tuple(min(1.0, c * tint) for c in STONE)
    dark = tuple(min(1.0, c * tint) for c in STONE_DARK)
    brick = box(name, (size[0] - 0.008, size[1], size[2] - 0.008),
                flat_material("forge_pierre", STONE), location=center)
    bevel(brick, 0.02, segments=2)
    return two_tone(brick, light, dark, f"forge_brique_{round(tint * 100)}",
                    dark_side=(0.3, 0.6, -1.0), threshold=0.1, spread=0.04)


def _wall(row, length, along_x, offset, rng):
    """Un rang de briques le long d'un mur ; un rang sur deux décalé d'une demi-brique."""
    count = round(length / BRICK[0])
    step = length / count
    shift = step / 2 if row % 2 else 0.0
    z = BRICK[2] * (row + 0.5)
    parts = []
    for i in range(count):
        t = -length / 2 + step * (i + 0.5) + shift
        if t > length / 2:
            continue
        x, y = (t, offset) if along_x else (offset, t)
        parts.append(_brick(f"brique_{row}_{along_x}_{offset:+.2f}_{i}", (x, y, z), along_x,
                            rng.uniform(0.9, 1.1)))
    return parts


def _hearth():
    """Maçonnerie lisse : cœur sombre, six rangs de briques en quinconce, margelle en pierre."""
    rng = random.Random(2)
    core = box("foyer", (HEARTH[0] - 0.1, HEARTH[1] - 0.1, HEARTH_TOP),
               flat_material("forge_coeur", STONE_DARK), location=(0, 0, HEARTH_TOP / 2))
    parts = [core]
    lx, ly = HEARTH[0] / 2 - BRICK[1] / 2, HEARTH[1] / 2 - BRICK[1] / 2
    for row in range(COURSES):
        parts += _wall(row, HEARTH[0], True, -ly, rng) + _wall(row, HEARTH[0], True, ly, rng)
        parts += (_wall(row, HEARTH[1] - 2 * BRICK[1], False, -lx, rng)
                  + _wall(row, HEARTH[1] - 2 * BRICK[1], False, lx, rng))
    return parts + _rim()


def _rim():
    """Margelle : quatre dalles arrondies autour du lit de braises."""
    z = COURSES * BRICK[2] + 0.03
    slab = flat_material("forge_margelle", STONE)
    pieces = [((HEARTH[0] + 0.04, 0.12, 0.06), (0, -HEARTH[1] / 2 + 0.04, z)),
              ((HEARTH[0] + 0.04, 0.12, 0.06), (0, HEARTH[1] / 2 - 0.04, z)),
              ((0.12, HEARTH[1] - 0.2, 0.06), (-HEARTH[0] / 2 + 0.04, 0, z)),
              ((0.12, HEARTH[1] - 0.2, 0.06), (HEARTH[0] / 2 - 0.04, 0, z))]
    parts = []
    for i, (size, center) in enumerate(pieces):
        piece = box(f"margelle_{i}", size, slab, location=center)
        bevel(piece, 0.025, segments=2)
        parts.append(two_tone(piece, (0.66, 0.62, 0.56), STONE_DARK, "forge_margelle",
                              dark_side=(0.3, 0.6, -1.0), threshold=0.1, seed=i))
    return parts


def _embers():
    """Lit de cendres et braises qui rougeoient sur le dessus du foyer."""
    rng = random.Random(3)
    parts = [box("cendres", (HEARTH[0] - 0.16, HEARTH[1] - 0.16, 0.03),
                 flat_material("forge_cendres", ASH), location=(0, 0, HEARTH_TOP + 0.005))]
    for i in range(14):
        color = EMBER_HOT if i % 3 == 0 else EMBER
        ember = rock(f"braise_{i}", (0.045, 0.04, 0.03),
                     flat_material(f"forge_braise_{i % 3 == 0}", color, emission=3.0),
                     count=8, dents=0, seed=60 + i)
        ember.location = (rng.uniform(-0.33, 0.33), rng.uniform(-0.24, 0.24), HEARTH_TOP + 0.03)
        parts.append(ember)
    return parts


def _crucible():
    """Creuset de fer posé sur les braises, plein de métal en fusion."""
    z = HEARTH_TOP + 0.03
    pot = cylinder("creuset", 0.11, 0.18, material("metal_sombre"), vertices=10,
                   location=(0.12, 0.0, z + 0.09), radius_top=0.135)
    blotches(pot, material("rouille"), count=2, radius=0.06, seed=5)
    molten = cylinder("metal_fondu", 0.12, 0.01,
                      flat_material("forge_fondu", MOLTEN, emission=5.0), vertices=10,
                      location=(0.12, 0.0, z + 0.17))
    return [pot, molten]


# --- Hotte et cheminée ---------------------------------------------------------------------------

def _hood():
    """Hotte en tôles dépareillées : chaque pan d'une couleur (récupérées de partout), rivets,
    poteaux qui la portent, tuyau de cheminée."""
    hood = cone("hotte", 0.72, 0.5, material("metal"), vertices=4,
                location=(0, 0, HOOD_BOTTOM + 0.25), rotation_deg=(0, 0, 45))
    hood.data.materials.append(material("rouille"))
    hood.data.materials.append(material("metal_sombre"))
    for i, polygon in enumerate(hood.data.polygons):
        polygon.material_index = i % 3  # pans dépareillés
    base = HOOD_BOTTOM + 0.4
    pipe_points = [(0, 0, base + 0.9 * i / 6) for i in range(7)]  # tronçons : rouille par plaques
    pipe = tube("cheminee", pipe_points, [0.1] * 7, material("metal_sombre"), sides=12)
    two_tone(pipe, TOOL_COLORS["metal_sombre"], (0.34, 0.33, 0.32), "forge_cheminee",
             dark_side=(0.4, 0.6, 0.0), seed=8)
    blotches(pipe, material("rouille"), count=3, radius=0.1, seed=8)
    collar = cylinder("cheminee_collier", 0.12, 0.05, material("metal"), vertices=12,
                      location=(0, 0, HOOD_BOTTOM + 0.5))
    return [hood, pipe, collar] + _posts()


def _posts():
    """Quatre poteaux de fer, des coins du foyer jusqu'à la hotte."""
    dark = material("metal_sombre")
    lx, ly = HEARTH[0] / 2 - 0.06, HEARTH[1] / 2 - 0.06
    return [tube(f"poteau_{sx}_{sy}", [(sx * lx, sy * ly, HEARTH_TOP),
                                       (sx * lx * 0.9, sy * ly * 0.9, HOOD_BOTTOM + 0.02)],
                 [0.018, 0.016], dark, sides=5)
            for sx in (-1, 1) for sy in (-1, 1)]


# --- Autour --------------------------------------------------------------------------------------

def _bellows():
    """Soufflet sur le côté gauche : deux planches, le cuir entre elles, la buse vers le foyer."""
    x = -HEARTH[0] / 2 - 0.38
    parts = posed(board("soufflet_haut", 0.6, 0.32, 0.03, seed=12, grain=1), (0, -8, 0), (x, 0, 0.58))
    parts += posed(board("soufflet_bas", 0.6, 0.32, 0.03, seed=13, grain=1), (0, 6, 0), (x, 0, 0.42))
    parts.append(box("soufflet_cuir", (0.5, 0.28, 0.12), flat_material("forge_cuir", LEATHER),
                     location=(x + 0.02, 0, 0.5)))
    parts.append(cylinder("soufflet_buse", 0.025, 0.25, material("metal_sombre"), vertices=6,
                          location=(x + 0.4, 0, 0.5), rotation_deg=(0, 90, 0)))
    parts.append(tube("soufflet_poignee", [(x - 0.3, 0, 0.6), (x - 0.42, 0, 0.66)], [0.015, 0.015],
                      material("bois_sombre"), sides=5))
    return parts + _bellows_stand(x)


def _bellows_stand(x):
    """Tréteau qui porte le soufflet."""
    parts = []
    for i, dx in enumerate((-0.2, 0.2)):
        leg = board(f"soufflet_pied_{i}", 0.4, 0.06, 0.06, ("poutre", "poutre_sombre"), seed=14 + i,
                    grain=0)
        parts += posed(leg, (0, 90, 0), (x + dx, 0, 0.2))
    return parts


ANVIL = [(-0.2, 0.0), (0.12, 0.0), (0.12, 0.1), (0.2, 0.18), (0.3, 0.22), (0.12, 0.25),
         (-0.22, 0.25), (-0.26, 0.22), (-0.13, 0.17), (-0.15, 0.08), (-0.22, 0.04)]


def _anvil():
    """Enclume (profil classique, avec sa bigorne) sur un billot, à droite du foyer."""
    x, y = HEARTH[0] / 2 + 0.42, -0.1
    stump = cylinder("billot", 0.19, 0.5, flat_material("forge_billot", (0.45, 0.32, 0.22)),
                     vertices=9, location=(x, y, 0.25))
    two_tone(stump, (0.50, 0.36, 0.24), (0.34, 0.24, 0.16), "forge_billot",
             dark_side=(0.4, 0.6, -0.2), seed=15)
    outline = [(x + px * 0.9, 0.5 + pz) for px, pz in ANVIL]
    anvil = profile_solid("enclume", outline, lambda _: 0.11, [material("metal_sombre")],
                          lambda _: 0, cuts_x=(x - 0.1, x + 0.1), chamfer=0.006)
    two_tone(anvil, (0.32, 0.32, 0.34), (0.20, 0.20, 0.22), "forge_enclume",
             dark_side=(0.3, 0.6, -1.0), seed=16)
    anvil.location.y = y
    return [stump, anvil]


def _coal_pile():
    """Tas de charbon au pied du foyer."""
    rng = random.Random(9)
    coal = flat_material("forge_charbon", COAL)
    parts = []
    for i in range(10):
        lump = rock(f"charbon_{i}", (0.05, 0.045, 0.035), coal, count=8, dents=0, seed=80 + i)
        lump.location = (rng.uniform(-0.15, 0.15) - 0.15, -HEARTH[1] / 2 - 0.18 + rng.uniform(-0.08, 0.08),
                         0.03 + (0.04 if i > 6 else 0))
        parts.append(lump)
    return parts


def build():
    return (_hearth() + _embers() + _crucible() + _hood() + _bellows() + _anvil()
            + _coal_pile())
