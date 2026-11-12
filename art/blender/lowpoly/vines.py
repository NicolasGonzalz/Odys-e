"""Lianes et lierre : la nature qui reprend les objets humains (coffres, terminaux, ruines).

Une liane suit une suite de points posés juste contre une surface : une tige fine et ondulée, et
des feuilles en losange, deux verts, réparties le long de la tige et tournées vers l'extérieur.
"""
import math
import random

from mathutils import Vector

from .materials import flat_material
from .organic import tube
from .shapes import cone

LEAF = (0.42, 0.62, 0.26)
LEAF_DARK = (0.27, 0.45, 0.18)
STEM = (0.32, 0.36, 0.18)


def _along(points, t):
    """Point à la fraction t (0 à 1) d'une ligne brisée."""
    segments = len(points) - 1
    i = min(int(t * segments), segments - 1)
    local = t * segments - i
    return points[i].lerp(points[i + 1], local)


def vine(name, points, outward, leaves=10, size=0.035, seed=0):
    """Liane le long de `points` (x, y, z), feuilles tournées vers `outward` (la normale de la
    surface sur laquelle elle pousse). Renvoie la tige et ses feuilles."""
    rng = random.Random(seed)
    path = [Vector(p) for p in points]
    away = Vector(outward).normalized()
    parts = [tube(f"{name}_tige", [tuple(p) for p in path],
                  [0.007 * (1 - 0.5 * i / max(1, len(path) - 1)) for i in range(len(path))],
                  flat_material("liane_tige", STEM), sides=4)]
    for i in range(leaves):
        parts.append(_leaf(f"{name}_feuille_{i}", _along(path, (i + 0.5) / leaves), away,
                           size * rng.uniform(0.75, 1.25), rng))
    return parts


def _leaf(name, base, away, size, rng):
    """Feuille en losange (cône aplati à 4 côtés), penchée hors de la surface, de travers."""
    color = LEAF if rng.random() < 0.6 else LEAF_DARK
    side = away.orthogonal().normalized()
    direction = (away * 0.8 + side * rng.uniform(-1.0, 1.0) + Vector((0, 0, rng.uniform(-0.3, 0.6))))
    direction.normalize()
    leaf = cone(name, size * 0.5, size, flat_material(f"liane_feuille_{color == LEAF}", color),
                vertices=4)
    leaf.scale = (1.4, 0.35, 1.0)  # feuille large, pas une épine
    leaf.location = base + direction * size * 0.5
    leaf.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    leaf.rotation_euler.z += rng.uniform(-math.pi, math.pi) * 0.2
    return leaf
