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


def save_blend(category, name):
    """Le .blend retouchable (sans copie de sauvegarde .blend1)."""
    blend = ART / "blender" / "sources" / category / f"{name}.blend"
    blend.parent.mkdir(parents=True, exist_ok=True)
    _make_writable(blend)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    return blend


def save_palette(image, category, name):
    """La texture-palette, à côté du .fbx (Unreal la retrouve à l'import) et dans le .blend."""
    path = ART / "exports" / category / f"{name}_palette.png"
    path.parent.mkdir(parents=True, exist_ok=True)
    _make_writable(path)
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    image.pack()
    return path


def export_model(obj, category, name):
    blend = save_blend(category, name)
    fbx = ART / "exports" / category / f"{name}.fbx"
    fbx.parent.mkdir(parents=True, exist_ok=True)
    _make_writable(fbx)

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
        path_mode="COPY",
        embed_textures=True,  # la palette voyage dans le .fbx
    )
    return blend, fbx
