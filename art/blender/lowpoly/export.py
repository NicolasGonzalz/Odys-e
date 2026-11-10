"""Écrit le modèle : .blend (à retoucher dans Blender) et .fbx (à importer dans Unreal)."""
import stat
from pathlib import Path

import bpy

ART = Path(__file__).resolve().parents[2]  # dossier art/


def _make_writable(path):
    """Les .blend sont « verrouillables » (Git LFS) et donc en lecture seule hors verrou :
    un fichier régénéré par script doit pouvoir être réécrit."""
    if path.exists():
        path.chmod(path.stat().st_mode | stat.S_IWUSR)


def export_model(obj, category, name):
    blend = ART / "blender" / "sources" / category / f"{name}.blend"
    fbx = ART / "exports" / category / f"{name}.fbx"
    blend.parent.mkdir(parents=True, exist_ok=True)
    fbx.parent.mkdir(parents=True, exist_ok=True)
    _make_writable(blend)
    _make_writable(fbx)

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
