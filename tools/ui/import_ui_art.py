"""Importe les visuels de l'interface (art/ui/, voir generate_ui_art.py) dans Unreal :
/Game/Odyssee/UI/Textures/T_Ui_<nom>. À exécuter DANS l'éditeur (Outils > Exécuter un script
Python…). Relançable : les textures existantes sont remplacées.
"""
from pathlib import Path

import unreal

FOLDER = "/Game/Odyssee/UI/Textures"


def _source_dir():
    try:
        return Path(__file__).resolve().parents[2] / "art" / "ui"
    except NameError:  # exécuté par l'exécution à distance : pas de __file__
        project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
        return project.parent.parent / "art" / "ui"


def _import(path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", f"T_Ui_{path.stem}")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{FOLDER}/T_Ui_{path.stem}")


def _as_ui_texture(texture):
    """Réglages d'une image d'interface : pleine résolution, sans mipmaps ni streaming."""
    texture.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def main():
    files = sorted(p for p in _source_dir().iterdir() if p.suffix in (".png", ".jpg"))
    for path in files:
        _as_ui_texture(_import(path))
    unreal.log(f"[Odyssée] interface : {len(files)} textures importées dans {FOLDER}")


main()
