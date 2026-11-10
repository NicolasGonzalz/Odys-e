"""Bois plus vrai, commun aux manches d'outils, à la bûche et à l'arbalète.

Sur les planches, le bois n'est pas une suite de facettes au hasard : ses tons courent en longues
veines dans le sens du fil, quelques traits sombres le parcourent, il a des nœuds et ses bouts
sciés montrent des cernes. On reste en couleurs unies (pas de texture), donc léger et low-poly.

  streaked      : tons en longues veines (même nuance tout le long d'une génératrice)
  surface_point : point posé sur la surface facettée d'un tube (organic.tube)
  grain_lines   : fines lignes sombres du fil, légèrement ondulées, plaquées sur un tube
  grain_strokes : mêmes traits sur une pièce plate, à partir de leurs points
  knot          : nœud (ovale sombre en léger relief, allongé dans le sens du fil)
  end_grain     : bout scié (cernes concentriques, cœur, fente)
"""
import math
import random

import bpy
from mathutils import Vector

from .materials import flat_material
from .organic import tube
from .outline import without_outline
from .shading import shade_variants

LINE_RADIUS = 0.0013  # rayon d'un trait du fil (m) : à moitié enfoncé, il se lit comme un trait
LINE_LIFT = 0.0003


# --- Tons ---------------------------------------------------------------------------------------

def streaked(obj, light, dark, name, axis=(0, 0, 1), dark_side=(1.0, -0.2, 0.0), threshold=0.25,
             streak=0.12, spread=0.14, seed=0):
    """Côté clair / côté sombre, et dans chacun 4 nuances réparties en veines : les faces d'une
    même génératrice gardent leur nuance sur environ `streak` m le long de `axis`."""
    light_set = shade_variants(f"{name}_clair", light, spread, count=4)
    dark_set = shade_variants(f"{name}_sombre", dark, spread, count=4)
    obj.data.materials.clear()
    for material in light_set + dark_set:
        obj.data.materials.append(material)

    axis, side = Vector(axis).normalized(), Vector(dark_side).normalized()
    for polygon in obj.data.polygons:
        offset = len(light_set) if polygon.normal.dot(side) > threshold else 0
        polygon.material_index = offset + _vein(polygon, axis, streak, seed, len(light_set))
    return obj


def _vein(polygon, axis, streak, seed, count):
    """Nuance d'une face : fixée par sa génératrice (angle autour de l'axe) et son tronçon."""
    u = axis.orthogonal().normalized()
    v = axis.cross(u)
    radial = polygon.normal - axis * polygon.normal.dot(axis)
    sector = int((math.atan2(radial.dot(v), radial.dot(u)) + math.pi) / (2 * math.pi) * 16)
    stretch = 1 + (sector * 7 + seed) % 5 / 4  # des veines plus longues que d'autres
    stretch_index = math.floor(polygon.center.dot(axis) / (streak * stretch))
    return random.Random(seed * 1009 + sector * 97 + stretch_index).randrange(count)


# --- Traits du fil ------------------------------------------------------------------------------

def surface_point(center_x, radius_at, z, theta, sides, flatten=1.0, lift=0.0):
    """Point de la surface facettée d'un tube vertical à `sides` pans (comme organic.tube) :
    entre deux arêtes, la surface est plate, plus proche de l'axe que le rayon."""
    step = 2 * math.pi / sides
    k = math.floor(theta / step)
    t = theta / step - k
    r = radius_at(z)
    x0, y0 = math.sin(k * step) * r, math.cos(k * step) * r * flatten
    x1, y1 = math.sin((k + 1) * step) * r, math.cos((k + 1) * step) * r * flatten
    x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
    norm = math.hypot(x, y) or 1.0
    return (center_x(z) + x + lift * x / norm, y + lift * y / norm, z)


def grain_strokes(name, strokes, color, radius=LINE_RADIUS):
    """Un trait sombre par liste de points (x, y, z)."""
    ink = flat_material(f"{name}_fil", color)
    return [without_outline(tube(f"{name}_fil_{i}", points, [radius] * len(points), ink, sides=3))
            for i, points in enumerate(strokes)]


def grain_lines(name, center_x, radius_at, z_bottom, z_top, color, sides=7, flatten=1.0,
                count=6, seed=0, radius=LINE_RADIUS):
    """Traits du fil répartis tout autour d'un manche, de longueurs variées, un peu ondulés."""
    rng = random.Random(seed)
    strokes = []
    for _ in range(count):
        theta = rng.uniform(0, 2 * math.pi)
        length = (z_top - z_bottom) * rng.uniform(0.35, 0.85)
        start = rng.uniform(z_bottom, z_top - length)
        wave = rng.uniform(-0.12, 0.12)
        strokes.append([surface_point(center_x, radius_at, start + length * f,
                                      theta + wave * math.sin(math.pi * f * 2), sides, flatten,
                                      LINE_LIFT) for f in (0, 0.33, 0.66, 1)])
    return grain_strokes(name, strokes, color, radius)


# --- Nœud ---------------------------------------------------------------------------------------

def knot(name, center_x, radius_at, z, theta, size, colors, sides=7, flatten=1.0):
    """Nœud du bois : ovale sombre au cœur plus foncé, en léger relief, allongé le long du fil.
    `colors` = (bord, cœur)."""
    position = Vector(surface_point(center_x, radius_at, z, theta, sides, flatten))
    outward = (position - Vector((center_x(z), 0, z))).normalized()
    bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=size, radius2=size * 0.55, depth=0.003)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.materials.append(flat_material(f"{name}_bord", colors[0]))
    obj.data.materials.append(flat_material(f"{name}_coeur", colors[1]))
    for polygon in obj.data.polygons:
        polygon.material_index = 1 if polygon.normal.z > 0.9 else 0
    obj.location = position
    obj.rotation_euler = outward.to_track_quat("Z", "Y").to_euler()
    obj.scale = (1.0, 1.8, 1.0)  # le Y local suit le manche : un nœud est allongé dans le fil
    return without_outline(obj)


# --- Bout scié ----------------------------------------------------------------------------------

def end_grain(name, center, normal, u_axis, radii, bands, sides=7, crack=None, lift=0.0005,
              seed=0):
    """Bout scié, posé à plat contre l'extrémité d'une pièce de bois.

    `u_axis` et `radii` = (rayon selon u, rayon selon v = normal × u) ; `bands` = liste
    (fraction du rayon, couleur) du cœur vers le bord, pour les cernes. `crack` = (angle, longueur
    en fraction) : une fente sombre qui part du cœur.
    """
    normal, u = Vector(normal).normalized(), Vector(u_axis).normalized()
    v = normal.cross(u)
    rng = random.Random(seed)
    ring_scales = [fraction * (1 + rng.uniform(-0.05, 0.05)) for fraction, _ in bands]

    def at(scale, k):
        angle = 2 * math.pi * k / sides
        return (Vector(center) + normal * lift + u * (math.cos(angle) * radii[0] * scale)
                + v * (math.sin(angle) * radii[1] * scale))

    vertices = [at(scale, k) for scale in ring_scales for k in range(sides)]
    faces = [tuple(range(sides))]
    for r in range(len(bands) - 1):
        faces += [(r * sides + k, (r + 1) * sides + k, (r + 1) * sides + (k + 1) % sides,
                   r * sides + (k + 1) % sides) for k in range(sides)]
    obj = _flat_mesh(name, vertices, faces, [color for _, color in bands], sides)
    parts = [obj]
    if crack:
        parts.append(_crack(f"{name}_fente", Vector(center) + normal * lift, normal, u, v, radii,
                            crack, bands[0][1]))
    return parts


def _flat_mesh(name, vertices, faces, colors, sides):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(p) for p in vertices], [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for index, color in enumerate(colors):
        obj.data.materials.append(flat_material(f"{name}_cerne_{index}", color))
    for index, polygon in enumerate(obj.data.polygons):
        polygon.material_index = 0 if index == 0 else 1 + (index - 1) // sides
    obj.data.update()
    return without_outline(_solid(obj))


def _crack(name, center, normal, u, v, radii, crack, color):
    """Fente : un mince triangle sombre du cœur vers le bord."""
    angle, length = crack
    direction = u * math.cos(angle) * radii[0] + v * math.sin(angle) * radii[1]
    across = normal.cross(direction).normalized() * 0.003
    base = Vector(center) + normal * 0.0009
    vertices = [base - across, base + direction * length, base + across]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(p) for p in vertices], [], [(0, 1, 2)])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(flat_material(f"{name}_noir", tuple(c * 0.45 for c in color)))
    return without_outline(_solid(obj))


def _solid(obj, thickness=0.0012):
    """Une pièce plate reçoit une épaisseur (vers l'intérieur du bois) : fermée, ses faces
    gardent une orientation sûre quand finish.outward_normals recalcule tout le modèle."""
    modifier = obj.modifiers.new("Epaisseur", "SOLIDIFY")
    modifier.thickness = thickness
    modifier.offset = -1.0
    return obj
