"""Rendu de vérification : le modèle en vue de trois quarts, couleurs unies, contours."""
import bpy
from mathutils import Vector


def _camera_for(obj):
    """Caméra orthographique de trois quarts, cadrée sur le modèle."""
    size = max(obj.dimensions)
    center = Vector((0, 0, obj.dimensions.z / 2))
    camera_data = bpy.data.cameras.new("Apercu")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = size * 1.35
    camera = bpy.data.objects.new("Apercu", camera_data)
    bpy.context.scene.collection.objects.link(camera)

    direction = Vector((1.0, -1.25, 0.75)).normalized()
    camera.location = center + direction * size * 4
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = camera


def _workbench(scene):
    """Moteur Workbench : rapide, fonctionne sans écran, montre les couleurs des matériaux."""
    scene.render.engine = "BLENDER_WORKBENCH"
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = "MATERIAL"
    shading.show_object_outline = True
    shading.show_cavity = True
    shading.cavity_type = "WORLD"
    shading.background_type = "VIEWPORT"
    shading.background_color = (0.36, 0.42, 0.50)  # le gris-bleu des planches de concepts
    scene.render.film_transparent = False


def render_preview(obj, path, resolution=900):
    scene = bpy.context.scene
    _camera_for(obj)
    _workbench(scene)
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return path

