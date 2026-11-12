"""Établi (poste d'artisanat) : on y travaille le bois et on assemble (docs/GAMEPLAY.md § 2) :
planches, arc, flèches, arbalète, coffres, table de cartographie, forge.

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/etabli.png).
Plateau de trois grosses planches inégales, grande tôle rouillée posée au milieu, étau à l'angle,
outils qui traînent (marteau, pince, lime, petite enclume, brique, engrenage, écrous, crayon),
pieds carrés en bois sombre, traverses boulonnées, chutes de tôle sur le bas.
1,4 × 0,7 m, plateau à 0,9 m. Pivot au pied.
"""
import math

from lowpoly.carpentry import board, sheet, stain
from lowpoly.finish import posed
from lowpoly.hammer import hammer
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import bevel, box, cylinder
from lowpoly.tool_parts import TOOL_COLORS, material

ICON = True
VIEW = (-0.4, -1.0, 0.6)    # de face, un peu à gauche et de haut, comme le concept

TOP = 0.90          # dessus du plateau
PLANK = 0.05        # épaisseur des planches du plateau
LEG_X, LEG_Y = 0.6, 0.27
POST = 0.08         # section des pieds
BRICK = (0.62, 0.30, 0.22)
PENCIL = (0.45, 0.75, 0.30)

# Planches du plateau : (y, longueur, décalage en x, surélévation, teinte, graine).
TOP_PLANKS = [(-0.24, 1.46, 0.02, 0.0, 1.0, 1), (0.0, 1.40, -0.01, 0.0, 0.92, 2),
              (0.24, 1.36, 0.0, 0.012, 1.06, 3)]

# Tôle du plateau, vue de dessus (x, y) : bords déchirés.
SHEET = [(-0.32, -0.30), (0.05, -0.33), (0.12, -0.22), (0.40, -0.26), (0.46, 0.05),
         (0.30, 0.18), (-0.05, 0.14), (-0.28, 0.20), (-0.36, -0.05)]


def _tabletop():
    parts = []
    for y, length, dx, lift, tint, seed in TOP_PLANKS:
        plank = board(f"plateau_{seed}", length, 0.235, PLANK, tint=tint, seed=seed, grain=4)
        parts += posed(plank, (0, 0, 0), (dx, y, TOP - PLANK / 2 + lift))
    parts += posed(sheet("plateau_tole", SHEET, rust=5, seed=4), (0, 0, 0), (0, 0, TOP + 0.003))
    for i, (x, y, r) in enumerate(((0.5, -0.12, 0.05), (-0.45, 0.2, 0.04), (0.55, 0.25, 0.035))):
        parts.append(stain(f"plateau_tache_{i}", (x, y, TOP + 0.0012 + (0.012 if y > 0.12 else 0)),
                           r, seed=i))
    return parts


def _frame():
    """Pieds, traverses hautes (sous le plateau) et basses (boulonnées)."""
    parts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            leg = board(f"pied_{sx}_{sy}", TOP - PLANK, POST, POST, ("poutre", "poutre_sombre"),
                        seed=5 + sx + 2 * sy, grain=1)
            parts += posed(leg, (0, 90, 0), (sx * LEG_X, sy * LEG_Y, (TOP - PLANK) / 2))
    for i, (z, name) in enumerate(((0.2, "basse"), (TOP - PLANK - 0.05, "haute"))):
        parts += _stretchers(name, z, seed=10 + i)
    return parts + _bolts()


def _stretchers(name, z, seed):
    parts = []
    for sy in (-1, 1):  # longues, devant et derrière
        beam = board(f"traverse_{name}_{sy}", 2 * LEG_X + 0.1, 0.1, 0.04, ("poutre", "poutre_sombre"),
                     seed=seed + sy, grain=1)
        parts += posed(beam, (90, 0, 0), (0, sy * (LEG_Y + POST / 2 + 0.02), z))
    for sx in (-1, 1):  # courtes, sur les côtés
        beam = board(f"traverse_{name}_cote_{sx}", 2 * LEG_Y + 0.1, 0.08, 0.035,
                     ("poutre", "poutre_sombre"), seed=seed + 3 + sx, grain=1)
        parts += posed(beam, (90, 0, 90), (sx * (LEG_X + POST / 2 + 0.018), 0, z))
    return parts


def _bolts():
    dark = flat_material("etabli_boulon", TOOL_COLORS["metal_sombre"])
    return [cylinder(f"boulon_{sx}_{sy}", 0.012, 0.012, dark, vertices=6,
                     location=(sx * LEG_X, sy * (LEG_Y + POST / 2 + 0.042), 0.2),
                     rotation_deg=(90, 0, 0))
            for sx in (-1, 1) for sy in (-1, 1)]


def _shelf_scrap():
    """Ce qui traîne en bas : deux tôles tordues et une planche cassée sur les traverses."""
    plates = (posed(sheet("bas_tole_0", [(-0.35, -0.2), (0.3, -0.24), (0.38, 0.12), (-0.3, 0.2)],
                          rust=3, seed=7), (6, 4, 18), (-0.05, 0.0, 0.25))
              + posed(sheet("bas_tole_1", [(-0.25, -0.15), (0.28, -0.1), (0.2, 0.16), (-0.3, 0.1)],
                            rust=2, seed=8), (-8, 3, -25), (0.15, 0.04, 0.28)))
    broken = posed(board("bas_planche", 0.55, 0.14, 0.025, tint=0.85, seed=9, grain=1),
                   (0, -8, 12), (0.1, -0.08, 0.31))
    return plates + broken


def _metal(part, seed, rust=2, radius=0.03):
    two_tone(part, TOOL_COLORS["metal"], TOOL_COLORS["metal_sombre"], "etabli_metal",
             dark_side=(0.3, 0.5, -1.0), threshold=0.2, seed=seed)
    return blotches(part, material("rouille"), count=rust, radius=radius, seed=seed)


def _vise():
    """Étau à l'angle avant gauche, bien visible : socle tournant, corps, deux mâchoires, vis qui
    sort vers l'avant et son levier en T."""
    x, y, z = -0.56, -0.25, TOP
    dark = material("metal_sombre")
    parts = [cylinder("etau_socle", 0.1, 0.03, dark, vertices=10, location=(x, y, z + 0.015))]
    body = box("etau_corps", (0.12, 0.2, 0.08), material("metal"), location=(x, y, z + 0.07))
    parts.append(_metal(bevel(body, 0.008), seed=19, rust=1, radius=0.02))
    for i, (dy, name) in enumerate(((0.07, "fixe"), (-0.1, "mobile"))):
        jaw = box(f"etau_machoire_{name}", (0.2, 0.06, 0.1), material("metal"),
                  location=(x, y + dy, z + 0.14))
        parts.append(_metal(bevel(jaw, 0.008), seed=20 + i, rust=1, radius=0.02))
    parts.append(cylinder("etau_vis", 0.016, 0.2, dark, vertices=6,
                          location=(x, y - 0.22, z + 0.11), rotation_deg=(90, 0, 0)))
    parts.append(cylinder("etau_levier", 0.009, 0.26, dark, vertices=6,
                          location=(x, y - 0.32, z + 0.11), rotation_deg=(0, 90, 20)))
    return parts


def _tools():
    """Les outils qui traînent sur le plateau."""
    parts = posed(hammer(seed=11), (90, 0, -35), (-0.12, 0.05, TOP + 0.016))
    parts += _pliers(0.3, -0.12)
    parts.append(_metal(bevel(box("enclume", (0.13, 0.1, 0.08), material("metal"),
                                  location=(-0.4, 0.2, TOP + 0.052)), 0.008), seed=30))
    parts.append(bevel(box("brique", (0.13, 0.065, 0.055), flat_material("brique", BRICK),
                           location=(-0.18, 0.24, TOP + 0.04), rotation_deg=(0, 0, -8)), 0.004))
    parts.append(box("lime", (0.2, 0.016, 0.006), material("metal_sombre"),
                     location=(0.42, -0.02, TOP + 0.006), rotation_deg=(0, 0, 70)))
    parts.append(cylinder("crayon", 0.005, 0.12, flat_material("crayon", PENCIL), vertices=6,
                          location=(0.28, 0.12, TOP + 0.008), rotation_deg=(0, 90, 15)))
    return parts + _gear(0.1, 0.18) + _nuts()


def _pliers(x, y):
    """Pince ouverte : deux branches qui se croisent."""
    dark = material("metal_sombre")
    z = TOP + 0.008
    return [tube(f"pince_{i}", [(x - 0.09, y + side * 0.03, z), (x, y + side * 0.004, z),
                                (x + 0.06, y - side * 0.012, z)], [0.006, 0.007, 0.005], dark, sides=5)
            for i, side in enumerate((-1, 1))]


def _gear(x, y):
    """Engrenage : un disque et ses dents."""
    metal = material("metal")
    z = TOP + 0.006
    parts = [cylinder("engrenage", 0.028, 0.008, metal, vertices=10, location=(x, y, z))]
    for i in range(8):
        angle = math.tau * i / 8
        parts.append(box(f"engrenage_dent_{i}", (0.014, 0.01, 0.008), metal,
                         location=(x + math.cos(angle) * 0.032, y + math.sin(angle) * 0.032, z),
                         rotation_deg=(0, 0, math.degrees(angle))))
    return parts


def _nuts():
    dark = material("metal_sombre")
    return [cylinder(f"ecrou_{i}", 0.011, 0.008, dark, vertices=6, location=(x, y, TOP + 0.006))
            for i, (x, y) in enumerate(((0.0, 0.2), (0.05, 0.24), (0.2, 0.22)))]


def build():
    return _tabletop() + _frame() + _shelf_scrap() + _vise() + _tools()
