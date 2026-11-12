"""Palette : toutes les couleurs d'un modèle dans une petite texture, un seul matériau.

Pour l'effet « peint à la main », les modèles sont construits avec un matériau uni par nuance
(des dizaines, voire des centaines : la forge en avait 282). Dans Unreal, chaque matériau coûte un
appel de dessin : un seul objet en vaudrait des centaines.

À l'export, bake() range chaque couleur dans une case d'une texture (4 × 4 pixels, filtrage au plus
proche : aucune couleur ne bave sur sa voisine) et fait pointer chaque face sur la case de sa
couleur. Le modèle n'a plus qu'un matériau (deux s'il a des parties lumineuses : braises, écrans),
et son rendu est identique.
"""
import math

import bpy

CELL = 4                 # pixels par case de couleur
GLOW_STRENGTH = 3.0      # intensité des parties lumineuses (à régler dans Unreal)


def _linear_to_srgb(c):
    return c * 12.92 if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055


def _color_of(material):
    """(couleur sRGB arrondie, lumineuse ?) d'un matériau uni (lowpoly.materials.flat_material)."""
    shader = next((n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if shader is None:
        rgb = tuple(material.diffuse_color[:3])
        glow = False
    else:
        rgb = tuple(shader.inputs["Base Color"].default_value[:3])
        glow = shader.inputs["Emission Strength"].default_value > 0
    srgb = tuple(round(_linear_to_srgb(max(0.0, min(1.0, c))), 3) for c in rgb)
    return srgb, glow


def _image(name, colors):
    """Texture carrée (puissance de 2) : une case de CELL × CELL pixels par couleur."""
    per_row = max(1, math.ceil(math.sqrt(len(colors))))
    size = 16
    while size < per_row * CELL:
        size *= 2
    pixels = [0.0] * (size * size * 4)
    for index, (r, g, b) in enumerate(colors):
        cx, cy = (index % per_row) * CELL, (index // per_row) * CELL
        for y in range(cy, cy + CELL):
            for x in range(cx, cx + CELL):
                offset = (y * size + x) * 4
                pixels[offset:offset + 4] = [r, g, b, 1.0]
    image = bpy.data.images.new(f"{name}_palette", size, size, alpha=False)
    image.pixels = pixels
    return image, per_row, size


def _material(name, image, glow):
    """Matériau : la texture-palette en couleur de base (et en émission s'il est lumineux)."""
    material = bpy.data.materials.new(f"M_{name}{'_lumineux' if glow else ''}")
    material.use_nodes = True
    material.use_backface_culling = True  # la coque du contour noir ne se voit que sur les bords
    nodes = material.node_tree.nodes
    shader = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    links = material.node_tree.links
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    shader.inputs["Roughness"].default_value = 0.85
    if glow:
        links.new(texture.outputs["Color"], shader.inputs["Emission Color"])
        shader.inputs["Emission Strength"].default_value = GLOW_STRENGTH
    return material


def _cell_uv(index, per_row, size):
    """Centre de la case n° `index`, en coordonnées de texture (0 à 1)."""
    x = (index % per_row) * CELL + CELL / 2
    y = (index // per_row) * CELL + CELL / 2
    return x / size, y / size


def bake(obj, name):
    """Remplace les matériaux unis de `obj` par une texture-palette. Renvoie l'image (à enregistrer)."""
    keys = [_color_of(m) for m in obj.data.materials]
    colors = sorted({srgb for srgb, _ in keys})
    image, per_row, size = _image(name, colors)

    uv = obj.data.uv_layers.new(name="palette") if not obj.data.uv_layers else obj.data.uv_layers[0]
    glows = []
    for polygon in obj.data.polygons:
        srgb, glow = keys[polygon.material_index]
        cell = _cell_uv(colors.index(srgb), per_row, size)
        for loop in polygon.loop_indices:
            uv.data[loop].uv = cell
        glows.append(glow)

    obj.data.materials.clear()
    obj.data.materials.append(_material(name, image, glow=False))
    if any(glows):
        obj.data.materials.append(_material(name, image, glow=True))
    for polygon, glow in zip(obj.data.polygons, glows):
        polygon.material_index = 1 if glow else 0
    return image
