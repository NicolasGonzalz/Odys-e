"""Assemblage d'un modèle : une seule pièce, pivot au pied, prête à exporter."""
import math

import bmesh
import bpy
from mathutils import Euler, Vector

from .outline import mark_outlined


def _select_only(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def join(objects, name):
    """Applique les modificateurs (biseaux…) puis fusionne les pièces en un seul objet."""
    for obj in objects:
        _select_only([obj])
        for modifier in list(obj.modifiers):
            bpy.ops.object.modifier_apply(modifier=modifier.name)
        mark_outlined(obj)
    _select_only(objects)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()
    model = bpy.context.active_object
    model.name = name
    model.data.name = name
    outward_normals(model)
    return model


def turned(objects, rotation_deg):
    """Tourne un ensemble de pièces autour de l'origine. Ex. : une bûche construite debout
    (comme un manche, selon z) puis couchée."""
    bpy.context.view_layer.update()  # matrices à jour pour les pièces placées par location
    rotation = Euler(tuple(math.radians(a) for a in rotation_deg)).to_matrix().to_4x4()
    for obj in objects:
        obj.matrix_world = rotation @ obj.matrix_world
    return objects


def posed(objects, rotation_deg, offset):
    """Tourne un ensemble de pièces autour de l'origine, puis le déplace de `offset` : pour poser
    une même pièce plusieurs fois (cartouches en tas, planches empilées)."""
    for obj in turned(objects, rotation_deg):
        obj.location = obj.location + Vector(offset)
    return objects


def outward_normals(obj):
    """Oriente toutes les faces vers l'extérieur. Indispensable : une face tournée vers l'intérieur
    disparaît quand les faces arrière sont masquées (Unreal, contour noir) et laisse un trou noir."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def origin_at_base(obj):
    """Pivot au centre du pied du modèle (posé au sol, z = 0), comme l'attend Unreal."""
    corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    base = Vector((sum(c.x for c in corners) / 8, sum(c.y for c in corners) / 8,
                   min(c.z for c in corners)))
    bpy.context.scene.cursor.location = base
    _select_only([obj])
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    obj.location = (0, 0, 0)
    return obj


def dimensions(obj):
    """Taille (x, y, z) en mètres, arrondie au centimètre (pour le rapport)."""
    return tuple(round(v, 2) for v in obj.dimensions)
