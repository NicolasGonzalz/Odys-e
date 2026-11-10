"""Matériaux de couleur unie, comme sur les planches de concepts (pas de texture)."""
import bpy

# Palette commune (rouge, vert, bleu entre 0 et 1), reprise des planches de art/concepts/.
PALETTE = {
    "bois": (0.40, 0.25, 0.13),
    "bois_clair": (0.62, 0.45, 0.28),
    "bois_sombre": (0.25, 0.15, 0.08),
    "metal": (0.50, 0.52, 0.55),
    "metal_sombre": (0.28, 0.29, 0.31),
    "rouille": (0.55, 0.28, 0.13),
    "tissu": (0.78, 0.72, 0.58),
    "cuir": (0.42, 0.24, 0.13),
    "feuillage": (0.30, 0.50, 0.17),
    "feuillage_sombre": (0.13, 0.33, 0.16),
    "pierre": (0.55, 0.55, 0.53),
    "mousse": (0.38, 0.55, 0.18),
}

_cache = {}


def srgb_to_linear(color):
    """Les couleurs sont données comme dans une image (sRGB) ; Blender calcule en linéaire.
    Sans cette conversion, un brun 0,48 s'afficherait comme un beige clair."""
    return tuple(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in color)


def _still_exists(material):
    """Un matériau mis en cache a pu être supprimé (scène remise à zéro entre deux modèles)."""
    try:
        return material.name in bpy.data.materials
    except ReferenceError:
        return False


def flat_material(name, color=None, emission=0.0, roughness=0.85):
    """Matériau uni. `name` peut être une entrée de PALETTE (la couleur est alors facultative)."""
    color = srgb_to_linear(color or PALETTE[name])
    key = (name, tuple(round(c, 3) for c in color), emission)
    if key in _cache and _still_exists(_cache[key]):
        return _cache[key]

    material = bpy.data.materials.new(f"M_{name}")
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1.0)
    shader.inputs["Roughness"].default_value = roughness
    if emission > 0:
        shader.inputs["Emission Color"].default_value = (*color, 1.0)
        shader.inputs["Emission Strength"].default_value = emission
    material.diffuse_color = (*color, 1.0)  # couleur affichée par le rendu de vérification
    _cache[key] = material
    return material
