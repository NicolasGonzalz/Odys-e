"""Bûche (ressource « bois ») : ce qui tombe quand on abat un arbre, et ce qu'on a dans l'inventaire.

Référence : art/concepts/objets/outils-et-ressources.png (découpe : references/buche.png).
Écorce brun foncé à grandes facettes, fissurée dans le sens du fil ; bouts sciés clairs à cernes,
fendus ; un moignon de branche sur le dessus. Couchée, longueur 0,44 m (tenue à une main).

Construite debout (selon z, comme un manche, pour réutiliser lowpoly/wood.py) puis couchée selon x.
"""
import math
import random

from mathutils import Vector

from lowpoly.finish import turned
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shapes import roughen
from lowpoly.tool_parts import TOOL_COLORS
from lowpoly.wood import end_grain, grain_lines, grain_strokes, streaked, surface_point

ICON = True
VIEW = (-0.8, -1.0, 0.6)    # trois quarts avant, bout scié visible comme sur le concept
ICON_VIEW = VIEW

LENGTH = 0.44
RADIUS = 0.088
SIDES = 9
TOP = 3 * math.pi / 2  # angle (surface_point) qui se retrouve sur le dessus une fois la bûche couchée

# Couleurs relevées sur le concept (sRGB).
BARK = (0.62, 0.43, 0.29)
BARK_DARK = (0.46, 0.31, 0.21)
FISSURE = (0.27, 0.17, 0.11)
RINGS = [(0.14, TOOL_COLORS["bois_coeur"]), (0.42, TOOL_COLORS["bois_bout"]),
         (0.52, TOOL_COLORS["bois_cerne"]), (0.84, TOOL_COLORS["bois_bout"]),
         (1.0, TOOL_COLORS["bois_cerne"])]


def _profile(seed):
    """Centre légèrement tordu et rayon irrégulier : un vrai tronçon de branche."""
    rng = random.Random(seed)
    zs = [LENGTH * i / 6 for i in range(7)]
    radii = [RADIUS * (1 + rng.uniform(-0.06, 0.06)) for _ in zs]
    radii[0], radii[-1] = RADIUS * 1.04, RADIUS * 0.97  # un bout un peu plus gros que l'autre

    def center_x(z):
        return 0.008 * math.sin(math.pi * z / LENGTH)

    def radius_at(z):
        i = min(int(z / LENGTH * 6), 5)
        t = z / LENGTH * 6 - i
        return radii[i] + (radii[i + 1] - radii[i]) * t
    return zs, center_x, radius_at


def _trunk(zs, center_x, radius_at):
    bark = flat_material("buche_ecorce", BARK)
    trunk = tube("tronc", [(center_x(z), 0.0, z) for z in zs], [radius_at(z) for z in zs], bark,
                 sides=SIDES)
    roughen(trunk, 0.002, seed=4)
    return streaked(trunk, BARK, BARK_DARK, "buche_ecorce", streak=0.09, spread=0.16, seed=4)


def _fissures(center_x, radius_at):
    """Fissures de l'écorce, plus une grande fente sur le côté (comme sur le concept)."""
    parts = grain_lines("ecorce", center_x, radius_at, 0.02, LENGTH - 0.02, FISSURE, SIDES,
                        count=9, seed=7, radius=0.0019)
    split = [surface_point(center_x, radius_at, z, TOP - 0.9, SIDES, lift=0.0004)
             for z in (0.04, 0.16, 0.27, 0.36)]
    return parts + grain_strokes("fente", [split], FISSURE, radius=0.0032)


def _sawn_ends(center_x, radius_at):
    """Les deux bouts sciés : cernes, cœur sombre, une fente qui part du cœur."""
    parts = []
    for index, (z, normal) in enumerate(((0.0, (0, 0, -1)), (LENGTH, (0, 0, 1)))):
        r = radius_at(z) * 0.97
        parts += end_grain(f"bout_{index}", (center_x(z), 0.0, z), normal, (0, 1, 0), (r, r), RINGS,
                           SIDES, crack=(0.5 + index * 2.4, 0.9), lift=0.0025, seed=index)
    return parts


def _branch_stub(center_x, radius_at):
    """Moignon de branche coupée sur le dessus, penché vers un bout, avec son bout scié."""
    z = LENGTH * 0.68
    base = Vector(surface_point(center_x, radius_at, z, TOP, SIDES))
    axis = Vector((center_x(z), 0.0, z))
    out = (base - axis).normalized()
    direction = (out + Vector((0, 0.15, 0.55))).normalized()
    tip = base + direction * 0.065
    stub = tube("branche", [tuple(axis + out * RADIUS * 0.5), tuple(base), tuple(tip)],
                [0.03, 0.027, 0.021], flat_material("buche_ecorce", BARK), sides=6)
    streaked(stub, BARK, BARK_DARK, "buche_branche", axis=tuple(direction), seed=2)
    u = direction.orthogonal().normalized()
    cut = end_grain("branche_bout", tuple(tip), tuple(direction), tuple(u), (0.02, 0.02),
                    RINGS[:3], 6, lift=0.001, seed=3)
    return [stub, *cut]


def build():
    zs, center_x, radius_at = _profile(seed=4)
    parts = ([_trunk(zs, center_x, radius_at)] + _fissures(center_x, radius_at)
             + _sawn_ends(center_x, radius_at) + _branch_stub(center_x, radius_at))
    return turned(parts, (0, 90, 0))  # couchée : le dessus (TOP) passe vers le haut
