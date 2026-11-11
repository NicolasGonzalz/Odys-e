"""Aperçu du relief seul, sans végétation ni lieux. À exécuter DANS l'éditeur Unreal.

Prérequis : le relief importé dans le niveau (Landscape depuis output/heightmap.png).

Pose seulement :
  - une couleur unie sur le Landscape (sinon il reste noir) ;
  - soleil, ciel, lumière du ciel, brouillard et nuages (s'ils manquent) ;
  - le plan d'eau au niveau 0 ;
  - la caméra de l'éditeur au-dessus du sud de la carte, tournée vers le nord.

Lancer : Outils > Exécuter un script Python… > ce fichier. Relançable sans doublon.
Tout ce qu'il pose porte l'étiquette « généré » : build_map.py le remplacera plus tard.
"""
import importlib
import json
import sys
from pathlib import Path

import unreal


def _script_dir():
    """Dossier de ce script (repli sur le chemin depuis le projet Unreal si __file__ manque)."""
    try:
        return Path(__file__).resolve().parent
    except NameError:
        project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
        return project.parent.parent / "tools" / "worldgen" / "unreal"


HERE = _script_dir()
WORLDGEN = HERE.parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))



def _load(name):
    return importlib.reload(importlib.import_module(f"odyssee_world.{name}"))


def _landscapes(util):
    return [a for a in util.actors().get_all_level_actors() if isinstance(a, unreal.LandscapeProxy)]


def _remove_previous_water(util):
    """Le plan d'eau est recréé à chaque lancement : on supprime l'ancien."""
    old = [a for a in util.actors().get_all_level_actors()
           if a.actor_has_tag(util.GENERATED_TAG) and a.actor_has_tag("eau")]
    if old:
        util.actors().destroy_actors(old)


def _paint_ground(util, landscapes, material):
    for landscape in landscapes:
        util.safe_set(landscape, "landscape_material", material)


def _world_size_m(landscape, layout):
    """Taille réelle de la carte : le plan est en pixels de heightmap (1 px = 1 m à l'échelle 100),
    le Landscape peut être agrandi (échelle X/Y > 100 cm par point)."""
    scale = landscape.get_actor_scale3d()
    world = layout["world"]
    return world["width_m"] * scale.x / 100.0, world["height_m"] * scale.y / 100.0


def _corner(landscape):
    """Coin nord-ouest du Landscape (sa position d'acteur), en mètres."""
    location = landscape.get_actor_location()
    return location.x / 100.0, location.y / 100.0


def _water(util, assets, landscape, layout):
    """Un plan d'eau au niveau 0 qui couvre exactement le Landscape (rivière, lac, mares)."""
    width, height = _world_size_m(landscape, layout)
    x0, y0 = _corner(landscape)
    water = util.spawn(unreal.StaticMeshActor, util.to_cm(x0 + width / 2, y0 + height / 2, 0),
                       "Eau — niveau 0", "Odyssee/Ambiance", ["eau"])
    water.set_actor_scale3d(unreal.Vector(width, height, 1))  # le plan d'origine fait 1 m
    water.static_mesh_component.set_static_mesh(assets.shape_mesh("plane"))
    water.static_mesh_component.set_material(0, assets.color_material((0.16, 0.36, 0.55)))
    util.safe_set(water, "is_spatially_loaded", False)  # toujours chargé (il couvre tout le monde)


def _frame_camera(util, landscape, layout):
    """Vue d'ensemble : depuis le sud de la carte, en hauteur, regard vers le nord."""
    width, height = _world_size_m(landscape, layout)
    x0, y0 = _corner(landscape)
    location = util.to_cm(x0 + width / 2, y0 + height + width * 0.4, width * 0.6)
    rotation = unreal.Rotator(0.0, -35.0, -90.0)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    editor.set_level_viewport_camera_info(location, rotation)


def main():
    util = _load("unreal_util")
    assets_module = _load("unreal_assets")
    ground = _load("unreal_ground")
    ambiance = _load("unreal_ambiance")

    landscapes = _landscapes(util)
    if not landscapes:
        unreal.log_error("[Odyssée] aucun Landscape dans ce niveau : importer d'abord "
                         "tools/worldgen/output/heightmap.png (voir tools/worldgen/README.md)")
        return

    layout = json.loads((WORLDGEN / "world_layout.json").read_text(encoding="utf-8"))
    assets = assets_module.AssetLibrary()

    # Matériau qui gère les trous (entrées de grotte percées par build_blockout.py).
    _paint_ground(util, landscapes, ground.ground_material(WORLDGEN))
    ambiance._sky()
    _remove_previous_water(util)
    main_landscape = next((l for l in landscapes if isinstance(l, unreal.Landscape)), landscapes[0])
    _water(util, assets, main_landscape, layout)
    _frame_camera(util, main_landscape, layout)
    width, height = _world_size_m(main_landscape, layout)
    unreal.log(f"[Odyssée] carte : {width:.0f} × {height:.0f} m, "
               f"diagonale {((width ** 2 + height ** 2) ** 0.5) / 1000:.1f} km")

    assets.save()
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[Odyssée] aperçu du relief prêt")


main()
