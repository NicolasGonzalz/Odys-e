"""Construit la base de la carte d'Odyssée dans Unreal. À exécuter DANS l'éditeur Unreal.

Prérequis (voir tools/worldgen/README.md) :
  1. le relief importé dans le niveau (Landscape depuis output/heightmap.png) ;
  2. le projet compilé (classe C++ OdysseeScatterActor) ;
  3. les plugins « Python Editor Script Plugin » et « Editor Scripting Utilities » actifs.

Lancer : niveau Lvl_Monde ouvert, puis Tools > Execute Python Script… > ce fichier.
Le script peut être relancé autant de fois que voulu : il supprime d'abord ce qu'il avait posé.
"""
import importlib
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

MODULES = ["layout", "terrain", "noise", "placement", "catalog", "scatter", "city", "plan",
           "unreal_util", "unreal_assets", "unreal_spawn", "unreal_ambiance"]


def _load_modules():
    """(Re)charge les modules : une modification est prise en compte sans redémarrer l'éditeur."""
    loaded = {}
    for name in MODULES:
        module = importlib.import_module(f"odyssee_world.{name}")
        loaded[name] = importlib.reload(module)
    return loaded


def _check_prerequisites():
    if not hasattr(unreal, "OdysseeScatterActor"):
        raise RuntimeError("Classe OdysseeScatterActor introuvable : compiler d'abord le projet "
                           "C++ (Source/Odysse/World/OdysseeScatterActor).")
    has_landscape = any(isinstance(a, unreal.LandscapeProxy) for a in
                        unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
                        .get_all_level_actors())
    if not has_landscape:
        unreal.log_warning("[Odyssée] aucun Landscape dans ce niveau : importer d'abord "
                           "tools/worldgen/output/heightmap.png (voir README).")


def main():
    m = _load_modules()
    _check_prerequisites()

    layout = m["layout"].Layout.load(WORLDGEN / "world_layout.json")
    terrain = m["terrain"].Terrain(WORLDGEN / "output")
    catalog = m["catalog"].load_catalog(HERE / "elements.json")
    rules = m["catalog"].load_rules(HERE / "scatter_rules.json")

    steps = 4 + len(catalog)
    with unreal.ScopedSlowTask(steps, "Odyssée : génération de la carte…") as task:
        task.make_dialog(True)

        task.enter_progress_frame(1, "Calcul du plan (végétation, ville, lieux)…")
        plan = m["plan"].build_plan(layout, terrain, catalog, rules)

        task.enter_progress_frame(1, "Suppression de la génération précédente…")
        removed = m["unreal_util"].clear_generated()

        assets = m["unreal_assets"].AssetLibrary()
        actor_count = 0
        for element_name, placements in plan.instances.items():
            if task.should_cancel():
                unreal.log_warning("[Odyssée] génération annulée")
                return
            task.enter_progress_frame(1, f"Pose : {element_name} ({len(placements)})")
            element = catalog[element_name]
            actor_count += m["unreal_spawn"].spawn_scatter(element, placements, assets)

        task.enter_progress_frame(1, "Lieux, boss, artefacts, cocons, apparition…")
        m["unreal_spawn"].spawn_labeled(plan.labeled, catalog, assets)
        m["unreal_spawn"].spawn_player_start(plan.spawn)

        task.enter_progress_frame(1, "Ambiance : ciel, eau, brumes…")
        m["unreal_ambiance"].build_ambiance(layout.data, assets)

        assets.save()
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()

    unreal.log(f"[Odyssée] carte générée : {plan.count()} éléments dans {actor_count} groupes, "
               f"{len(plan.labeled)} acteurs nommés ({removed} anciens acteurs supprimés).")


main()
