"""Bâton (ressource) : une branche droite ramassée au sol ou coupée sur un arbre. Avec un caillou,
c'est le tout premier outil (site : « Tout part d'un bâton et d'un caillou ») : manches, flèches,
torches.

Pas de concept dédié : même bois que la bûche (lowpoly/wood.py), écorce plus fine et plus grise
(une jeune branche), un moignon de rameau, un bout coupé net et l'autre cassé en biseau.
Couché, longueur 0,55 m. Construit debout (selon z) puis couché selon x, comme la bûche.
"""
import math
import random

from mathutils import Vector

from lowpoly.finish import turned
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shapes import roughen
from lowpoly.tool_parts import TOOL_COLORS
from lowpoly.wood import end_grain, grain_lines, streaked, surface_point

ICON = True
VIEW = (-0.5, -1.0, 0.7)
ICON_VIEW = VIEW

LENGTH = 0.55
RADIUS = 0.016
SIDES = 7
TOP = 3 * math.pi / 2  # angle (surface_point) qui se retrouve sur le dessus une fois couché

BARK = (0.55, 0.43, 0.32)       # jeune écorce : plus grise que celle de la bûche
BARK_DARK = (0.40, 0.31, 0.23)
FISSURE = (0.30, 0.22, 0.15)
SLANT = 0.6   # pente de la cassure de la pointe, tournée vers la caméra (-y)
RINGS = [(0.25, TOOL_COLORS["bois_coeur"]), (0.85, TOOL_COLORS["bois_bout"]),
         (1.0, TOOL_COLORS["bois_cerne"])]


def _profile(seed):
    """Légère courbe et rayon qui s'affine vers la pointe : une vraie branche, pas un tourillon."""
    rng = random.Random(seed)
    zs = [LENGTH * i / 8 for i in range(9)]
    radii = [RADIUS * (1.12 - 0.3 * i / 8) * (1 + rng.uniform(-0.05, 0.05)) for i in range(9)]

    def center_x(z):
        return 0.012 * math.sin(math.pi * z / LENGTH) + 0.004 * math.sin(3 * math.pi * z / LENGTH)

    def radius_at(z):
        i = min(int(z / LENGTH * 8), 7)
        t = z / LENGTH * 8 - i
        return radii[i] + (radii[i + 1] - radii[i]) * t
    return zs, center_x, radius_at


def _branch(zs, center_x, radius_at):
    """La branche et ses fines fissures d'écorce."""
    points = [(center_x(z), 0.0, z) for z in zs]
    stick = tube("baton", points, [radius_at(z) for z in zs], flat_material("baton_ecorce", BARK),
                 sides=SIDES)
    _break_tip(stick)
    roughen(stick, 0.0008, seed=3)
    streaked(stick, BARK, BARK_DARK, "baton_ecorce", streak=0.07, spread=0.15, seed=3)
    lines = grain_lines("baton", center_x, radius_at, 0.03, LENGTH - 0.03, FISSURE, SIDES,
                        count=6, seed=5, radius=0.001)
    return [stick, *lines]


def _break_tip(stick):
    """Incline le dernier anneau du tube : la pointe est cassée en biseau, pas coupée droit.
    La coupe regarde vers -y, le côté vu par l'aperçu et l'icône."""
    for vertex in stick.data.vertices[-SIDES:]:
        vertex.co.z += SLANT * vertex.co.y
    stick.data.update()


def _ends(center_x, radius_at):
    """Bout du pied coupé net (cernes) ; bout de la pointe cassé en biseau (bois clair)."""
    r = radius_at(0.0) * 0.95
    cut = end_grain("bout", (center_x(0.0), 0.0, 0.0), (0, 0, -1), (0, 1, 0), (r, r), RINGS, SIDES,
                    lift=0.0008, seed=1)
    tip = Vector((center_x(LENGTH), 0.0, LENGTH))
    slant = Vector((0.0, -SLANT, 1.0)).normalized()  # le plan incliné par _break_tip
    r = radius_at(LENGTH) * 0.9
    stretch = math.sqrt(1 + SLANT * SLANT)  # le biseau allonge la coupe dans le sens de la pente
    broken = end_grain("cassure", tuple(tip), tuple(slant), (1, 0, 0), (r, r * stretch), RINGS[1:],
                       SIDES, lift=0.0008, seed=2)
    return cut + broken


def _twig(center_x, radius_at):
    """Moignon de rameau coupé, sur le dessus, penché vers la pointe."""
    z = LENGTH * 0.58
    base = Vector(surface_point(center_x, radius_at, z, TOP, SIDES))
    out = (base - Vector((center_x(z), 0.0, z))).normalized()
    direction = (out + Vector((0.0, 0.0, 1.1))).normalized()
    tip = base + direction * 0.04
    twig = tube("rameau", [tuple(base - out * 0.006), tuple(tip)], [0.007, 0.005],
                flat_material("baton_ecorce", BARK), sides=5)
    streaked(twig, BARK, BARK_DARK, "baton_rameau", axis=tuple(direction), seed=4)
    u = direction.orthogonal().normalized()
    cut = end_grain("rameau_bout", tuple(tip), tuple(direction), tuple(u), (0.0045, 0.0045),
                    RINGS[:2], 5, lift=0.0006, seed=3)
    return [twig, *cut]


def build():
    zs, center_x, radius_at = _profile(seed=6)
    parts = _branch(zs, center_x, radius_at) + _ends(center_x, radius_at) + _twig(center_x, radius_at)
    return turned(parts, (0, 90, 0))  # couché : le dessus (TOP) passe vers le haut
