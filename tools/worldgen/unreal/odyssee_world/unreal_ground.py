"""Matériau du sol, style low-poly : facettes nettes, couleurs de la carte (output/colormap.png),
roche sur les pentes raides, neige en altitude, et trous possibles (entrées de grotte)."""
from pathlib import Path

import unreal

from .unreal_util import safe_set

FOLDER = "/Game/Odyssee/Generated"
TEXTURE_NAME = "T_OdysseeSolCouleurs"
MATERIAL_NAME = "M_OdysseeSolFacettes"

WORLD_SIZE_CM = (403200.0, 302400.0)  # 4032 x 3024 m : la carte de couleurs couvre tout le monde
ROCK = (0.17, 0.16, 0.14)             # couleurs linéaires (≈ sRGB 115, 110, 100)
SNOW = (0.80, 0.84, 0.90)
STEEP_BELOW = 0.80                    # normale.z sous 0,8 (pente > ~37°) : roche
SNOW_FROM_CM = 22000.0                # neige à partir de 220 m, pleine à 250 m


def ground_material(worldgen_dir):
    """Importe la carte de couleurs et (re)construit le matériau. Renvoie le matériau."""
    texture = _import_colormap(Path(worldgen_dir) / "output" / "colormap.png")
    material = _empty_material()
    _build_graph(material, texture)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def _import_colormap(path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", TEXTURE_NAME)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(f"{FOLDER}/{TEXTURE_NAME}")
    safe_set(texture, "address_x", unreal.TextureAddress.TA_CLAMP)
    safe_set(texture, "address_y", unreal.TextureAddress.TA_CLAMP)
    safe_set(texture, "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    # Groupe « UI » : celui par défaut (World) plafonne les textures à 2048 px, soit 2 m par pixel
    # ici, et rend les chemins flous. La carte fait 4033 px : 1 m par pixel.
    safe_set(texture, "lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    safe_set(texture, "filter", unreal.TextureFilter.TF_BILINEAR)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def _empty_material():
    """Le matériau existant, vidé (le Landscape garde sa référence), ou un nouveau."""
    path = f"{FOLDER}/{MATERIAL_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        material = unreal.EditorAssetLibrary.load_asset(path)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            MATERIAL_NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("tangent_space_normal", False)  # normale donnée en monde
    return material


class _Graph:
    """Petit assistant pour poser des nœuds et les relier."""

    def __init__(self, material):
        self.material = material
        self.lib = unreal.MaterialEditingLibrary
        self.y = 0

    def node(self, cls, x, **props):
        expression = self.lib.create_material_expression(self.material, cls, x, self.y)
        self.y += 90
        for name, value in props.items():
            expression.set_editor_property(name, value)
        return expression

    def link(self, source, target, target_input="", source_output=""):
        self.lib.connect_material_expressions(source, source_output, target, target_input)

    def output(self, source, prop, source_output=""):
        self.lib.connect_material_property(source, source_output, prop)


def _mask(graph, source, x, **channels):
    node = graph.node(unreal.MaterialExpressionComponentMask, x, r=channels.get("r", False),
                      g=channels.get("g", False), b=channels.get("b", False), a=False)
    graph.link(source, node)
    return node


def _flat_normal(graph, position):
    """Normale de la facette, à partir des dérivées écran de la position : une seule normale par
    triangle, donc des faces nettes."""
    ddx = graph.node(unreal.MaterialExpressionDDX, -1400)
    ddy = graph.node(unreal.MaterialExpressionDDY, -1400)
    graph.link(position, ddx)
    graph.link(position, ddy)
    cross = graph.node(unreal.MaterialExpressionCrossProduct, -1200)
    # Ordre ddx × ddy : avec le repère main gauche d'Unreal, la normale pointe vers le haut.
    graph.link(ddx, cross, "A")
    graph.link(ddy, cross, "B")
    normal = graph.node(unreal.MaterialExpressionNormalize, -1000)
    graph.link(cross, normal)
    return normal


def _colormap_sample(graph, position, texture):
    xy = _mask(graph, position, -1400, r=True, g=True)
    uv = graph.node(unreal.MaterialExpressionDivide, -1200)
    size = graph.node(unreal.MaterialExpressionConstant2Vector, -1400, r=WORLD_SIZE_CM[0],
                      g=WORLD_SIZE_CM[1])
    graph.link(xy, uv, "A")
    graph.link(size, uv, "B")
    sample = graph.node(unreal.MaterialExpressionTextureSample, -1000, texture=texture)
    graph.link(uv, sample, "UVs")
    return sample


def _ramp(graph, source, offset, scale, x):
    """saturate((source + offset) × scale) : 0 → 1 sur une petite plage."""
    add = graph.node(unreal.MaterialExpressionAdd, x, const_b=offset)
    graph.link(source, add, "A")
    mul = graph.node(unreal.MaterialExpressionMultiply, x + 150, const_b=scale)
    graph.link(add, mul, "A")
    sat = graph.node(unreal.MaterialExpressionSaturate, x + 300)
    graph.link(mul, sat)
    return sat


def _lerp(graph, a, b, alpha, x):
    node = graph.node(unreal.MaterialExpressionLinearInterpolate, x)
    graph.link(a, node, "A")
    graph.link(b, node, "B")
    graph.link(alpha, node, "Alpha")
    return node


def _color(graph, rgb, x):
    return graph.node(unreal.MaterialExpressionConstant3Vector, x,
                      constant=unreal.LinearColor(*rgb, 1.0))


def _build_graph(material, texture):
    graph = _Graph(material)
    position = graph.node(unreal.MaterialExpressionWorldPosition, -1600)
    normal = _flat_normal(graph, position)
    up = _mask(graph, normal, -800, b=True)
    height = _mask(graph, position, -800, b=True)

    # Roche quand la facette est raide (normale.z < 0,8), neige en altitude sur les faces douces.
    steep = _ramp(graph, up, -STEEP_BELOW, -10.0, -700)
    high = _ramp(graph, height, -SNOW_FROM_CM, 1.0 / 3000.0, -700)
    flat_enough = _ramp(graph, up, -0.75, 8.0, -700)
    snowy = graph.node(unreal.MaterialExpressionMultiply, -300)
    graph.link(high, snowy, "A")
    graph.link(flat_enough, snowy, "B")

    ground = _colormap_sample(graph, position, texture)
    rocky = _lerp(graph, ground, _color(graph, ROCK, -400), steep, -200)
    base = _lerp(graph, rocky, _color(graph, SNOW, -400), snowy, 0)

    graph.output(base, unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(normal, unreal.MaterialProperty.MP_NORMAL)
    roughness = graph.node(unreal.MaterialExpressionConstant, 0, r=0.92)
    graph.output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    holes = graph.node(unreal.MaterialExpressionLandscapeVisibilityMask, 0)
    graph.output(holes, unreal.MaterialProperty.MP_OPACITY_MASK)
