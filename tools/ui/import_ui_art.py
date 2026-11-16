"""Importe les visuels de l'interface (art/ui/, voir generate_ui_art.py) dans Unreal :
/Game/Odyssee/UI/Textures/T_Ui_<nom>, puis les icônes d'objets (art/exports/icones/) en
T_Ui_objet_<nom> (le nom de base du modèle de l'objet, data/items.txt). À exécuter DANS l'éditeur
(Outils > Exécuter un script Python…).

Par défaut, seules les textures absentes sont importées : les autres sont des fichiers Git LFS en
lecture seule, qu'il ne faut pas réenregistrer pour rien. Après avoir changé une image existante,
mettre REIMPORT à True (et rendre ses fichiers modifiables).
"""
from pathlib import Path

import unreal

FOLDER = "/Game/Odyssee/UI/Textures"
REIMPORT = False


def _art_dir():
    try:
        return Path(__file__).resolve().parents[2] / "art"
    except NameError:  # exécuté par l'exécution à distance : pas de __file__
        project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
        return project.parent.parent / "art"


def _import(path, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{FOLDER}/{name}")


def _as_ui_texture(texture):
    """Réglages d'une image d'interface : pleine résolution, sans mipmaps ni streaming."""
    texture.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def main():
    art = _art_dir()
    ui = sorted(p for p in (art / "ui").iterdir() if p.suffix in (".png", ".jpg"))
    icons = sorted((art / "exports" / "icones").glob("*.png"))
    wanted = [(p, f"T_Ui_{p.stem}") for p in ui] + [(p, f"T_Ui_objet_{p.stem}") for p in icons]
    todo = [(p, name) for p, name in wanted
            if REIMPORT or not unreal.EditorAssetLibrary.does_asset_exist(f"{FOLDER}/{name}")]
    for path, name in todo:
        _as_ui_texture(_import(path, name))
    unreal.log(f"[Odyssée] interface : {len(todo)} textures importées sur {len(wanted)} dans {FOLDER}")


main()
