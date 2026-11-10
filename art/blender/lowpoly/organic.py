"""Formes organiques low-poly : tubes qui suivent une courbe, volumes construits par tranches.

C'est ce qui évite l'aspect « tout droit » : manches courbés, pattes pliées aux articulations,
corps d'animaux plus larges au poitrail et fins à la taille.
"""
import math

import bpy
from mathutils import Vector


def _ring(center, side_axis, up_axis, half_width, half_height, sides, twist=0.0):
    """Sommets d'une tranche elliptique autour de `center`."""
    points = []
    for k in range(sides):
        angle = 2 * math.pi * k / sides + twist
        points.append(center + side_axis * (math.cos(angle) * half_width)
                      + up_axis * (math.sin(angle) * half_height))
    return points


def _mesh_from_rings(name, rings, material):
    """Relie les tranches entre elles et ferme les deux bouts."""
    sides = len(rings[0])
    vertices = [tuple(p) for ring in rings for p in ring]
    faces = []
    for r in range(len(rings) - 1):
        for k in range(sides):
            a, b = r * sides + k, r * sides + (k + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.append(tuple(reversed(range(sides))))                        # bout de départ
    faces.append(tuple(range((len(rings) - 1) * sides, len(rings) * sides)))  # bout d'arrivée

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material)
    return obj


def _frame(tangent):
    """Deux axes perpendiculaires à la direction `tangent` (pour orienter une tranche)."""
    up = Vector((0, 0, 1)) if abs(tangent.z) < 0.9 else Vector((1, 0, 0))
    side = tangent.cross(up).normalized()
    return side, side.cross(tangent).normalized()


def tube(name, points, radii, material, sides=6):
    """Tube passant par `points` (x, y, z), avec un rayon par point : il suit la courbe."""
    points = [Vector(p) for p in points]
    rings = []
    for i, point in enumerate(points):
        before = points[max(i - 1, 0)]
        after = points[min(i + 1, len(points) - 1)]
        side, up = _frame((after - before).normalized())
        rings.append(_ring(point, side, up, radii[i], radii[i], sides))
    return _mesh_from_rings(name, rings, material)


def loft(name, sections, material, sides=8):
    """Volume orienté selon x, défini par des tranches (x, z_centre, demi-largeur, demi-hauteur).

    Exemple : un corps d'animal, du bassin (x négatif) au poitrail (x positif).
    """
    side_axis, up_axis = Vector((0, 1, 0)), Vector((0, 0, 1))
    rings = [_ring(Vector((x, 0, z)), side_axis, up_axis, width, height, sides, math.pi / sides)
             for x, z, width, height in sections]
    return _mesh_from_rings(name, rings, material)


def mirrored(build_side):
    """Construit une pièce des deux côtés (y = +1 et y = -1) : pattes, oreilles, yeux."""
    return [build_side(+1), build_side(-1)]
