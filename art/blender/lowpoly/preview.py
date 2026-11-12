"""Rendu de vérification : le modèle en vue de trois quarts, couleurs unies, contours."""
import bpy
from mathutils import Vector


DEFAULT_VIEW = (1.0, -1.25, 0.75)  # trois quarts avant, légèrement de haut


def _camera_for(obj, view, margin=1.35):
    """Caméra orthographique cadrée sur le modèle, regardant depuis la direction `view`."""
    size = max(obj.dimensions)
    center = Vector((0, 0, obj.dimensions.z / 2))
    camera_data = bpy.data.cameras.new("Apercu")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = size * margin
    camera = bpy.data.objects.new("Apercu", camera_data)
    bpy.context.scene.collection.objects.link(camera)

    direction = Vector(view).normalized()
    camera.location = center + direction * size * 4
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = camera


def _workbench(scene, color_type):
    """Moteur Workbench : rapide, fonctionne sans écran. color_type : MATERIAL (couleurs unies)
    ou TEXTURE (modèles générés par IA, dont les couleurs sont dans une image)."""
    scene.render.engine = "BLENDER_WORKBENCH"
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = color_type
    shading.show_object_outline = True
    shading.show_backface_culling = True  # la coque du contour noir ne se voit que sur les bords
    shading.show_cavity = True
    shading.cavity_type = "WORLD"
    shading.background_type = "VIEWPORT"
    shading.background_color = (0.36, 0.42, 0.50)  # le gris-bleu des planches de concepts
    scene.render.film_transparent = False


def render_preview(obj, path, resolution=900, color_type="MATERIAL", view=DEFAULT_VIEW):
    scene = bpy.context.scene
    _camera_for(obj, view)
    _workbench(scene, color_type)
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return path



def render_icon(obj, path, view=DEFAULT_VIEW, resolution=256):
    """Icône d'inventaire : le modèle seul sur fond transparent (PNG RGBA), cadré serré."""
    scene = bpy.context.scene
    _camera_for(obj, view, margin=1.12)
    _workbench(scene, "TEXTURE")  # les couleurs sont dans la texture-palette
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    scene.render.film_transparent = False
    scene.render.image_settings.color_mode = "RGB"
    return path
