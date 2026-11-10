"""Écrit le modèle : .blend (à retoucher dans Blender) et .fbx (à importer dans Unreal)."""
from pathlib import Path

import bpy

ART = Path(__file__).resolve().parents[2]  # dossier art/


def export_model(obj, category, name):
    blend = ART / "blender" / "sources" / category / f"{name}.blend"
    fbx = ART / "exports" / category / f"{name}.fbx"
    blend.parent.mkdir(parents=True, exist_ok=True)
    fbx.parent.mkdir(parents=True, exist_ok=True)

    bpy.ops.wm.save_as_mainfile(filepath=str(blend))

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=str(fbx),
        use_selection=True,
        apply_scale_options="FBX_SCALE_UNITS",  # mètres Blender → centimètres Unreal
        axis_forward="-Y",
        axis_up="Z",
        mesh_smooth_type="FACE",  # facettes nettes (low-poly)
        bake_space_transform=True,
    )
    return blend, fbx
