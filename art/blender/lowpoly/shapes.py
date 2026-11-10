"""Formes de base low-poly. Dimensions en mètres, positions au centre de la forme."""
import math
import random

import bmesh
import bpy


def _finish(obj, material, location, rotation_deg):
    obj.location = location
    obj.rotation_euler = tuple(math.radians(a) for a in rotation_deg)
    obj.data.materials.append(material)
    return obj


def box(name, size, material, location=(0, 0, 0), rotation_deg=(0, 0, 0)):
    """Boîte de dimensions `size` = (x, y, z)."""
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    return _finish(obj, material, location, rotation_deg)


def cylinder(name, radius, depth, material, vertices=8, location=(0, 0, 0), rotation_deg=(0, 0, 0),
             radius_top=None):
    """Cylindre (ou tronc de cône si `radius_top` diffère) vertical, peu de faces."""
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius,
                                    radius2=radius if radius_top is None else radius_top,
                                    depth=depth)
    obj = bpy.context.active_object
    obj.name = name
    return _finish(obj, material, location, rotation_deg)


def cone(name, radius, depth, material, vertices=8, location=(0, 0, 0), rotation_deg=(0, 0, 0)):
    return cylinder(name, radius, depth, material, vertices, location, rotation_deg, radius_top=0)


def blob(name, radius, material, subdivisions=1, scale=(1, 1, 1), location=(0, 0, 0),
         rotation_deg=(0, 0, 0)):
    """Sphère à facettes (icosphère) : feuillages, rochers, corps d'animaux."""
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, radius=radius)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(scale=True)
    return _finish(obj, material, location, rotation_deg)


def taper(obj, axis, keep_side, amount):
    """Amincit une forme d'un côté (ex. le tranchant d'une lame) : les sommets du côté
    opposé à `keep_side` sur `axis` sont rapprochés du centre sur les autres axes."""
    mesh = obj.data
    others = [i for i in range(3) if i != axis]
    for vertex in mesh.vertices:
        if (vertex.co[axis] > 0) != (keep_side > 0):
            for i in others[:1]:
                vertex.co[i] *= amount
    mesh.update()
    return obj


def roughen(obj, amount, seed):
    """Déplace un peu chaque sommet au hasard : look « taillé à la main » (pierre, bois)."""
    rng = random.Random(seed)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for vertex in bm.verts:
        vertex.co.x += rng.uniform(-amount, amount)
        vertex.co.y += rng.uniform(-amount, amount)
        vertex.co.z += rng.uniform(-amount, amount)
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def bevel(obj, width, segments=1):
    """Chanfrein sur les arêtes : attrape la lumière comme sur les planches."""
    modifier = obj.modifiers.new("Bevel", "BEVEL")
    modifier.width = width
    modifier.segments = segments
    modifier.limit_method = "ANGLE"
    return obj


def wedge(name, length, back, front, material, location=(0, 0, 0), rotation_deg=(0, 0, 0)):
    """Boîte qui change de section le long de x (lames, socs, museaux…).

    `back` et `front` = (épaisseur y, hauteur z) à l'arrière (x = 0) et à l'avant (x = length).
    """
    (by, bz), (fy, fz) = back, front
    corners = [(0, -by / 2, -bz / 2), (0, by / 2, -bz / 2),
               (0, by / 2, bz / 2), (0, -by / 2, bz / 2),
               (length, -fy / 2, -fz / 2), (length, fy / 2, -fz / 2),
               (length, fy / 2, fz / 2), (length, -fy / 2, fz / 2)]
    faces = [(0, 1, 2, 3), (7, 6, 5, 4), (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(corners, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return _finish(obj, material, location, rotation_deg)

