"""Pose le plan dans le niveau : végétation par carrés, acteurs nommés, point d'apparition."""
import math
from collections import defaultdict

import unreal

from .unreal_util import disable_collision, spawn, to_cm

CHUNK_M = 256  # un acteur de végétation par élément et par carré de 256 m (World Partition)


# --- Transformations --------------------------------------------------------------------------

def _part_transform(placement, part):
    """Position d'une forme provisoire : décalée et agrandie selon l'élément, puis tournée."""
    sx, sy, sz = placement.scale
    ox, oy, oz = part.offset[0] * sx, part.offset[1] * sy, part.offset[2] * sz
    yaw = math.radians(placement.yaw_deg)
    dx = ox * math.cos(yaw) - oy * math.sin(yaw)
    dy = ox * math.sin(yaw) + oy * math.cos(yaw)
    return unreal.Transform(
        location=to_cm(placement.x + dx, placement.y + dy, placement.z + oz),
        rotation=unreal.Rotator(0.0, 0.0, placement.yaw_deg),
        scale=unreal.Vector(part.size[0] * sx, part.size[1] * sy, part.size[2] * sz))


def _real_transform(placement):
    """Vrai modèle : pivot au pied, taille d'origine multipliée par la variation aléatoire."""
    return unreal.Transform(location=to_cm(placement.x, placement.y, placement.z),
                            rotation=unreal.Rotator(0.0, 0.0, placement.yaw_deg),
                            scale=unreal.Vector(*placement.scale))


def _visuals(element, assets):
    """Liste de (modèle, matériau, fonction de transformation) pour un élément."""
    if element.has_real_mesh:
        mesh = assets.real_mesh(element.mesh)
        if mesh is not None:
            return [(mesh, None, _real_transform)]
    return [(assets.shape_mesh(part.shape),
             assets.color_material(part.color, element.emissive),
             lambda placement, part=part: _part_transform(placement, part))
            for part in element.parts]


# --- Végétation et immeubles (instanciés) -----------------------------------------------------

def _chunks(placements):
    groups = defaultdict(list)
    for placement in placements:
        groups[(int(placement.x // CHUNK_M), int(placement.y // CHUNK_M))].append(placement)
    return groups


def spawn_scatter(element, placements, assets):
    """Un OdysseeScatterActor par carré et par forme : renvoie le nombre d'acteurs créés."""
    created = 0
    for (cx, cy), group in _chunks(placements).items():
        center = to_cm((cx + 0.5) * CHUNK_M, (cy + 0.5) * CHUNK_M, 0)
        for index, (mesh, material, transform) in enumerate(_visuals(element, assets)):
            label = f"{element.name} [{cx},{cy}] {index}"
            folder = f"Odyssee/Vegetation/{element.name}"
            actor = spawn(unreal.OdysseeScatterActor, center, label, folder,
                          ["scatter", element.name])
            actor.set_instances(mesh, material, [transform(p) for p in group])
            actor.set_instances_collision(element.collision)
            actor.set_instances_cull_distance(element.cull_m * 100.0)
            created += 1
    return created


# --- Acteurs nommés (lieux, boss, artefacts, cocons) ------------------------------------------

def spawn_labeled(labeled, catalog, assets):
    """Une forme par acteur, avec un nom lisible dans l'Outliner."""
    for item in labeled:
        element = catalog[item.placement.element]
        for mesh, material, transform in _visuals(element, assets):
            t = transform(item.placement)
            actor = spawn(unreal.StaticMeshActor, t.translation, item.label, item.folder, item.tags,
                          t.rotation.rotator())
            actor.set_actor_scale3d(t.scale3d)
            component = actor.static_mesh_component
            component.set_static_mesh(mesh)
            if material is not None:
                component.set_material(0, material)
            if not element.collision:
                disable_collision(component)


def spawn_player_start(position):
    x, y, z = position
    spawn(unreal.PlayerStart, to_cm(x, y, z), "Apparition — Centre de stase", "Odyssee/Joueurs",
          ["apparition"])
