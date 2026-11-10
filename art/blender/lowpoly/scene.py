"""Scène vide en mètres, prête à recevoir un modèle."""
import bpy


def reset_scene():
    """Supprime tout (objets, maillages, matériaux) et règle les unités en mètres."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    return scene
