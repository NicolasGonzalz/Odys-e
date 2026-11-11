"""Plan du blockout, sans Unreal : où va chaque bloc, chaque lumière, chaque trou du Landscape.

Une seule source pour deux usages : build_blockout.py pose ces blocs dans l'éditeur, et
export_world.py les écrit dans le fichier monde du serveur (collisions). Les deux voient donc
exactement les mêmes murs.
"""
from dataclasses import dataclass, field

from . import blockout

SPAWN_PLACE = "centre_stase"


@dataclass
class Group:
    """Blocs et lumières d'un même lieu, dans le repère du lieu (blockout.place)."""
    label: str
    folder: str
    origin: tuple  # (x, y, z) monde, en mètres : le sol du lieu
    yaw: float  # degrés, de +x vers +y
    boxes: list
    lights: list = field(default_factory=list)


@dataclass
class Hole:
    """Rectangle du Landscape à percer (couche de visibilité), en mètres."""
    center: tuple  # (x, y) monde
    length: float  # le long de `yaw`
    width: float
    yaw: float


@dataclass
class Plan:
    groups: list
    holes: list
    spawn: tuple  # (x, y, z) monde, en mètres : les pieds du joueur
    spawn_yaw: float


def blockout_plan(layout, terrain, meta):
    """Le centre de stase et les grottes. `terrain` : odyssee_world.terrain.Terrain ; `meta` :
    output/terrain_meta.json (sols des grottes et du bunker)."""
    stasis, stasis_hole, spawn = _stasis(layout, terrain, meta["stasis_floor_m"])
    caves, cave_holes = _caves(layout, terrain, meta["cave_floors_m"])
    place = next(p for p in layout.places if p["id"] == SPAWN_PLACE)
    return Plan([stasis] + caves, cave_holes + [stasis_hole], spawn, place["yaw_deg"])


def world_box(group, box):
    """Centre (m) et cap (degrés) d'un bloc dans le monde."""
    return blockout.place(box.center, group.origin, group.yaw), group.yaw + box.yaw


def _stasis(layout, terrain, floor):
    place = next(p for p in layout.places if p["id"] == SPAWN_PLACE)
    origin, yaw = (*place["pos"], floor), place["yaw_deg"]
    boxes, lights, player = blockout.stasis_center(_relief_around(terrain, place, floor))
    group = Group("Centre de stase", "Odyssee/Blockout/Centre de stase", origin, yaw, boxes,
                  lights)
    along, half_width = blockout.stasis_hole()
    return group, _hole(place, along, half_width), blockout.place(player, origin, yaw)


def _caves(layout, terrain, floors):
    shapes = layout.data["cave_shapes"]
    groups, holes = [], []
    for index, cave in enumerate(layout.data.get("caves", []), start=1):
        floor = floors[cave["id"]]
        label = f"Grotte {index:02d}"
        boxes, lights = blockout.cave(label, shapes, cave["type"],
                                      _relief_around(terrain, cave, floor))
        groups.append(Group(label, f"Odyssee/Blockout/Grottes/{cave['id']}",
                            (*cave["pos"], floor), cave["yaw_deg"], boxes, lights))
        along, half_width = blockout.cave_hole(shapes, cave["type"],
                                               _hole_length(terrain, cave, floor, shapes))
        holes.append(_hole(cave, along, half_width))
    return groups, holes


def _hole_length(terrain, cave, floor, shapes):
    """Longueur de la sortie du tunnel hors du relief : jusqu'à ce que le relief passe au-dessus
    du plafond du tunnel (on échantillonne l'axe et les deux bords)."""
    tunnel = shapes[cave["type"]]["tunnel"]
    ceiling = floor + tunnel[2] + 0.5
    for step in range(int(tunnel[0]) + 1):
        points = [blockout.place((-step, b, 0), (*cave["pos"], 0), cave["yaw_deg"])
                  for b in (-tunnel[1] / 2, 0, tunnel[1] / 2)]
        if min(terrain.height(px, py) for px, py, _ in points) > ceiling:
            return step + 1.0
    return float(tunnel[0])


def _relief_around(terrain, spot, floor):
    """Hauteur du terrain au-dessus de `floor` autour d'une grotte ou d'un lieu, en coordonnées
    locales (a vers la sortie)."""
    def relief(a, b):
        px, py, _ = blockout.place((a, b, 0), (*spot["pos"], 0), spot["yaw_deg"])
        return terrain.height(px, py) - floor
    return relief


def _hole(spot, along, half_width):
    """Rectangle local `along` × ±half_width autour de `spot`, en coordonnées monde."""
    middle = blockout.place(((along[0] + along[1]) / 2, 0, 0), (*spot["pos"], 0),
                            spot["yaw_deg"])
    return Hole(middle[:2], along[1] - along[0], 2 * half_width, spot["yaw_deg"])
