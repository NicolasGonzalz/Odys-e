"""Prépare les modèles générés par IA (art/ia3d/generated/<nom>.glb) pour le jeu.

Usage : blender -b --python art/blender/import_ia.py -- hache loup_gaia
Pour chaque modèle : import, fusion en une pièce, mise à la hauteur voulue (models.json),
pivot au pied, puis .blend, .fbx (art/exports/) et aperçu (art/blender/apercus/<nom>_ia.png).
"""
import json
import sys
from pathlib import Path

import bpy

HERE = Path(__file__).resolve().parent
ART = HERE.parent
sys.path.insert(0, str(HERE))

from lowpoly.export import export_model  # noqa: E402
from lowpoly.finish import dimensions, join, origin_at_base  # noqa: E402
from lowpoly.preview import render_preview  # noqa: E402
from lowpoly.scene import reset_scene  # noqa: E402


def load_specs():
    data = json.loads((ART / "ia3d" / "models.json").read_text(encoding="utf-8"))
    return {name: spec for name, spec in data.items() if not name.startswith("_")}


def import_glb(path):
    bpy.ops.import_scene.gltf(filepath=str(path))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"aucun maillage dans {path}")

    # Les .glb ont souvent des parents vides qui portent la position : on les détache
    # en gardant la position finale, pour que la fusion ne perde rien.
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
    return meshes


def scale_to_height(obj, height_m):
    """Le modèle IA arrive à une taille arbitraire : on le remet à sa vraie hauteur."""
    factor = height_m / max(obj.dimensions.z, 1e-6)
    obj.scale = (factor, factor, factor)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(scale=True)
    return obj


def prepare(name, spec):
    reset_scene()
    glb = ART / "ia3d" / "generated" / f"{name}.glb"
    model = join(import_glb(glb), name)
    model = origin_at_base(scale_to_height(model, spec["height_m"]))
    export_model(model, spec["category"], name)
    preview = render_preview(model, HERE / "apercus" / f"{name}_ia.png", color_type="TEXTURE")
    print(f"[odyssee] {spec['category']}/{name} (IA) : {dimensions(model)} m, "
          f"{len(model.data.polygons)} faces -> {preview.name}")


def main():
    names = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    specs = load_specs()
    for name in names:
        prepare(name, specs[name])


main()
