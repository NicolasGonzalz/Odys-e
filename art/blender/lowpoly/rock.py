"""Roches low-poly à grandes facettes, comme sur les planches (pierre, minerai, rochers de décor).

Des points irréguliers sur un ellipsoïde, leur enveloppe convexe (les grandes facettes), puis
quelques creux et une base aplatie pour que la roche tienne posée au sol.
"""
import math
import random

import bmesh
import bpy

GOLDEN_ANGLE = math.pi * (3 - math.sqrt(5))


def _points(size, count, jitter, rng):
    """Points répartis régulièrement sur un ellipsoïde (spirale de Fibonacci), puis bousculés."""
    points = []
    for i in range(count):
        z = 1 - 2 * (i + 0.5) / count
        ring = math.sqrt(1 - z * z)
        angle = GOLDEN_ANGLE * i
        scale = 1 + rng.uniform(-jitter, jitter)
        points.append((math.cos(angle) * ring * size[0] * scale,
                       math.sin(angle) * ring * size[1] * scale, z * size[2] * scale))
    return points


def _flatten_base(points, size, base):
    """Base aplatie à `base` × la demi-hauteur sous le centre : la roche ne roule pas."""
    floor = -size[2] * base
    return [(x, y, max(z, floor)) for x, y, z in points]


def _dent(bm, count, floor, rng):
    """Creuse quelques sommets (hors base) vers l'axe : un vrai caillou n'est pas convexe."""
    upper = [v for v in bm.verts if v.co.z > floor]
    for vertex in rng.sample(upper, min(count, len(upper))):
        vertex.co.x *= 0.8
        vertex.co.y *= 0.8


def rock(name, size, material, count=22, jitter=0.22, dents=2, base=0.6, facets=0.0, seed=0):
    """Roche de demi-axes `size` = (x, y, z), en mètres. `facets` (radians) fusionne les triangles
    presque coplanaires en grandes facettes planes, comme sur les planches. Renvoie l'objet."""
    rng = random.Random(seed)
    bm = bmesh.new()
    for point in _flatten_base(_points(size, count, jitter, rng), size, base):
        bm.verts.new(point)
    hull = bmesh.ops.convex_hull(bm, input=bm.verts[:])
    loose = {v for v in hull["geom_interior"] + hull["geom_unused"]
             if isinstance(v, bmesh.types.BMVert)}
    bmesh.ops.delete(bm, geom=list(loose), context="VERTS")
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=size[2] * 0.05)
    _dent(bm, dents, -size[2] * base + 1e-4, rng)
    if facets > 0:
        bmesh.ops.dissolve_limit(bm, angle_limit=facets, verts=bm.verts[:], edges=bm.edges[:])
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material)
    return obj


def on_surface(obj, x, y):
    """Point du dessus de la roche à la verticale de (x, y) : pour y poser un détail."""
    hit, location, _, _ = obj.ray_cast((x, y, 1.0), (0, 0, -1))
    return tuple(location) if hit else (x, y, 0.0)
