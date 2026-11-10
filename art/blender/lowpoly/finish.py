"""Assemblage d'un modèle : une seule pièce, pivot au pied, prête à exporter."""
import bpy
from mathutils import Vector


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
    _select_only(objects)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()
    model = bpy.context.active_object
    model.name = name
    model.data.name = name
    return model


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
