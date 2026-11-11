"""Blockout de la carte (blocs gris, sans végétation). À exécuter DANS l'éditeur Unreal.

Prérequis : le relief importé (Landscape) ; idéalement relief_preview.py déjà lancé (ciel, eau).

Pose :
  1. le centre de stase enterré sous sa butte (salle des capsules, couloir, sortie dans une
     entaille, esplanade) et l'apparition dedans ;
  2. les lieux, boss, artefacts et cocons en formes provisoires (comme build_map.py) ;
  3. les grottes de world_layout.json (« caves »), cachées dans le relief : tunnel et salle sous le
     terrain, lumières et point de loot ; Landscape percé à la main sur la face de la falaise.

Après une modification du relief (generate_terrain.py), réimporter d'abord heightmap.png dans le
Landscape (Mode Paysage > Gestion > Importer) : le sol des grottes suit la nouvelle heightmap.

Tout reste chargé en jeu (pas de streaming) pour pouvoir juger les distances en marchant.
Lancer : Outils > Exécuter un script Python… > ce fichier. Relançable : il remplace son blockout.
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

BLOCKOUT_TAG = "blockout"
PLAYER_START_LIFT_M = 1.0  # un PlayerStart Unreal se place au centre de la capsule, pas aux pieds


def _load(name):
    return importlib.reload(importlib.import_module(f"odyssee_world.{name}"))


def _clear_blockout(util):
    old = [a for a in util.actors().get_all_level_actors()
           if a.actor_has_tag(BLOCKOUT_TAG) or a.actor_has_tag("apparition")]
    if old:
        util.actors().destroy_actors(old)


def _spawn_box(util, assets, group, box):
    """Un bloc (cube ou cylindre de 1 m agrandi), à sa place dans le monde."""
    (x, y, z), yaw = m["blockout_plan"].world_box(group, box)
    actor = util.spawn(unreal.StaticMeshActor, util.to_cm(x, y, z), box.label, group.folder,
                       [BLOCKOUT_TAG], unreal.Rotator(0.0, 0.0, yaw))
    actor.set_actor_scale3d(unreal.Vector(*box.size))
    component = actor.static_mesh_component
    component.set_static_mesh(assets.shape_mesh(box.shape))
    component.set_material(0, assets.color_material(box.color, box.emissive))
    if not box.collision:
        util.disable_collision(component)
    util.safe_set(actor, "is_spatially_loaded", False)


def _spawn_light(util, group, light):
    x, y, z = m["blockout"].place(light.center, group.origin, group.yaw)
    actor = util.spawn(unreal.PointLight, util.to_cm(x, y, z), light.label, group.folder,
                       [BLOCKOUT_TAG])
    component = actor.get_component_by_class(unreal.PointLightComponent)
    r, g, b = (int(c * 255) for c in light.color)
    util.safe_set(component, "light_color", unreal.Color(r=r, g=g, b=b, a=255))
    util.safe_set(component, "attenuation_radius", light.radius_m * 100.0)
    util.safe_set(component, "intensity", 30.0)  # candelas : salle sombre mais lisible
    util.safe_set(actor, "is_spatially_loaded", False)


def _spawn_plan(util, assets, plan):
    """Blocs et lumières du centre de stase et des grottes, puis l'apparition du joueur."""
    for group in plan.groups:
        for box in group.boxes:
            _spawn_box(util, assets, group, box)
        for light in group.lights:
            _spawn_light(util, group, light)
    x, y, z = plan.spawn
    start = util.spawn(unreal.PlayerStart, util.to_cm(x, y, z + PLAYER_START_LIFT_M),
                       "Apparition — Centre de stase",
                       "Odyssee/Joueurs", ["apparition", BLOCKOUT_TAG],
                       unreal.Rotator(0, 0, plan.spawn_yaw))
    util.safe_set(start, "is_spatially_loaded", False)


def _places_and_markers(util, assets, layout, terrain):
    """Lieux, boss, artefacts et cocons en formes provisoires (le centre de stase a son blockout)."""
    plan = m["plan"]
    catalog = m["catalog"].load_catalog(HERE / "elements.json")
    labeled = [item for item in plan._places(layout, terrain) + plan._markers(layout, terrain)
               if m["blockout_plan"].SPAWN_PLACE not in item.tags]
    for item in labeled:
        item.tags = [*item.tags, BLOCKOUT_TAG]
    m["unreal_spawn"].spawn_labeled(labeled, catalog, assets)
    for actor in util.actors().get_all_level_actors():
        if actor.actor_has_tag(BLOCKOUT_TAG):
            util.safe_set(actor, "is_spatially_loaded", False)
    return len(labeled)


def _unreal_hole(hole):
    """Trou du plan (m) → unreal.OdysseeHole (cm)."""
    return unreal.OdysseeHole(center=unreal.Vector2D(hole.center[0] * 100, hole.center[1] * 100),
                              size=unreal.Vector2D(hole.length * 100, hole.width * 100),
                              yaw_degrees=hole.yaw)


def _apply_hole_brush(util, holes):
    """Un pinceau OdysseeHoleBrush (C++) perce toutes les entrées dans la couche de visibilité."""
    landscape = next(a for a in util.actors().get_all_level_actors()
                     if isinstance(a, unreal.Landscape))
    brush = util.spawn(unreal.OdysseeHoleBrush, unreal.Vector(0, 0, 0), "Trous des grottes",
                       "Odyssee/Blockout/Grottes", [BLOCKOUT_TAG])
    brush.attach_to_landscape(landscape)
    brush.set_holes(holes)


def main():
    global m
    m = {name: _load(name) for name in
         ("layout", "terrain", "catalog", "placement", "city", "scatter", "plan", "blockout",
          "blockout_plan", "unreal_util", "unreal_assets", "unreal_spawn", "unreal_ground")}
    util = m["unreal_util"]
    layout = m["layout"].Layout.load(WORLDGEN / "world_layout.json")
    terrain = m["terrain"].Terrain(WORLDGEN / "output")
    meta = json.loads((WORLDGEN / "output" / "terrain_meta.json").read_text(encoding="utf-8"))
    assets = m["unreal_assets"].AssetLibrary()

    _clear_blockout(util)
    plan = m["blockout_plan"].blockout_plan(layout, terrain, meta)
    _spawn_plan(util, assets, plan)
    places = _places_and_markers(util, assets, layout, terrain)
    _apply_hole_brush(util, [_unreal_hole(hole) for hole in plan.holes])

    _use_landscape_material_with_holes(util, assets)

    assets.save()
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log(f"[Odyssée] blockout posé : centre de stase, {places} lieux et marqueurs, "
               f"{len(plan.holes)} entrées percées (pinceau OdysseeHoleBrush)")


def _use_landscape_material_with_holes(util, assets):
    """Les trous ne s'affichent qu'avec un matériau qui lit la couche de visibilité."""
    material = m["unreal_ground"].ground_material(WORLDGEN)
    for landscape in util.actors().get_all_level_actors():
        if isinstance(landscape, unreal.LandscapeProxy):
            util.safe_set(landscape, "landscape_material", material)


main()
