#!/usr/bin/env python3
"""Fichier monde du serveur : output/odyssee.world (format : docs/WORLD_FORMAT.md, version 1).

Tout ce que la simulation doit savoir de la carte, sans Unreal : le relief (output/heightmap.png,
le même que celui importé dans le Landscape), les cases percées (entrées de grotte et du centre de
stase), les blocs de collision du blockout et les points d'intérêt (apparition, lieux, grottes).

Blocs et trous viennent de unreal/odyssee_world/blockout_plan.py, comme dans l'éditeur : le
serveur et Unreal voient exactement les mêmes murs.

Lancer après generate_terrain.py (depuis la racine du dépôt) :
    python3 tools/worldgen/export_world.py
"""
import json
import math
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
OUTPUT = HERE / "output"
sys.path.insert(0, str(HERE / "unreal"))

from odyssee_world import blockout_plan  # noqa: E402 (chemin ajouté juste au-dessus)
from odyssee_world.layout import Layout  # noqa: E402
from odyssee_world.terrain import Terrain  # noqa: E402

VERSION = 1
UNITS_PER_METER = 256
CHUNK_QUADS = 64
KIND_SPAWN, KIND_PLACE, KIND_CAVE = 0, 1, 2
SHAPES = {"cube": 0, "cylinder": 1}


# --- Unités de la simulation (comme net::quantizePosition et net::quantizeYaw) ----------------

def units(meters):
    return int(round(meters * UNITS_PER_METER))


def yaw_units(degrees):
    turns = math.fmod(degrees, 360.0) / 360.0
    return int(round((turns + 1.0 if turns < 0 else turns) * 65536)) & 0xFFFF


# --- Sections ----------------------------------------------------------------------------------

def terrain_section(heights):
    """Relief par blocs de CHUNK_QUADS cases, chaque bloc avec ses points de bord ; au-delà de la
    carte, les blocs du bord répètent la dernière rangée de points."""
    rows, cols = heights.shape
    quads_x, quads_y = cols - 1, rows - 1
    chunks_x, chunks_y = -(-quads_x // CHUNK_QUADS), -(-quads_y // CHUNK_QUADS)
    padded = np.pad(heights, ((0, chunks_y * CHUNK_QUADS + 1 - rows),
                              (0, chunks_x * CHUNK_QUADS + 1 - cols)), mode="edge")
    side = CHUNK_QUADS + 1
    chunks = [padded[y:y + side, x:x + side]
              for y in range(0, chunks_y * CHUNK_QUADS, CHUNK_QUADS)
              for x in range(0, chunks_x * CHUNK_QUADS, CHUNK_QUADS)]
    header = struct.pack("<IIIHHII", quads_x, quads_y, UNITS_PER_METER, CHUNK_QUADS, 0,
                         chunks_x, chunks_y)
    return header + b"".join(chunk.astype("<u2").tobytes() for chunk in chunks)


def hole_quads(holes, shape):
    """Cases percées : une case l'est dès qu'un de ses 4 points est dans un rectangle (le
    Landscape retire tous les triangles qui touchent un point masqué)."""
    rows, cols = shape
    hidden = np.zeros(shape, dtype=bool)
    for hole in holes:
        _hide_points(hidden, hole)
    quads = hidden[:-1, :-1] | hidden[:-1, 1:] | hidden[1:, :-1] | hidden[1:, 1:]
    quad_rows, quad_cols = np.nonzero(quads)  # déjà dans l'ordre ligne par ligne
    return list(zip(quad_cols.tolist(), quad_rows.tolist()))


def _hide_points(hidden, hole):
    reach = math.hypot(hole.length, hole.width) / 2 + 1
    (cx, cy), yaw = hole.center, math.radians(hole.yaw)
    x0, x1 = max(0, int(cx - reach)), min(hidden.shape[1], int(cx + reach) + 2)
    y0, y1 = max(0, int(cy - reach)), min(hidden.shape[0], int(cy + reach) + 2)
    ys, xs = np.mgrid[y0:y1, x0:x1]
    dx, dy = xs - cx, ys - cy
    along = dx * math.cos(yaw) + dy * math.sin(yaw)
    across = -dx * math.sin(yaw) + dy * math.cos(yaw)
    inside = (np.abs(along) <= hole.length / 2) & (np.abs(across) <= hole.width / 2)
    hidden[y0:y1, x0:x1] |= inside


def holes_section(quads):
    return struct.pack("<I", len(quads)) + b"".join(struct.pack("<II", c, r) for c, r in quads)


def collision_boxes(plan):
    """(centre, demi-tailles, cap, forme) en unités de la simulation, pour chaque bloc solide."""
    boxes = []
    for group in plan.groups:
        for box in group.boxes:
            if not box.collision:
                continue
            center, yaw = blockout_plan.world_box(group, box)
            half = [max(1, units(size / 2)) for size in box.size]
            boxes.append(([units(v) for v in center], half, yaw_units(yaw), SHAPES[box.shape]))
    return boxes


def boxes_section(boxes):
    records = [struct.pack("<iiiIIIHBB", *center, *half, yaw, shape, 0)
               for center, half, yaw, shape in boxes]
    return struct.pack("<I", len(boxes)) + b"".join(records)


def markers(layout, plan, meta, terrain):
    """Apparition, lieux (au niveau du sol) et entrées de grotte (au niveau de leur sol)."""
    found = [(KIND_SPAWN, plan.spawn, plan.spawn_yaw, blockout_plan.SPAWN_PLACE)]
    for place in layout.places:
        x, y = place["pos"]
        stasis = place["id"] == blockout_plan.SPAWN_PLACE
        floor = meta["stasis_floor_m"] if stasis else terrain(x, y)
        found.append((KIND_PLACE, (x, y, floor), place.get("yaw_deg", 0.0), place["id"]))
    for cave in layout.data.get("caves", []):
        found.append((KIND_CAVE, (*cave["pos"], meta["cave_floors_m"][cave["id"]]),
                      cave["yaw_deg"], cave["id"]))
    return found


def markers_section(found):
    records = []
    for kind, position, yaw, identifier in found:
        name = identifier.encode("utf-8")
        records.append(struct.pack("<BiiiH", kind, *(units(v) for v in position), yaw_units(yaw))
                       + struct.pack("<H", len(name)) + name)
    return struct.pack("<I", len(found)) + b"".join(records)


# --- Fichier ------------------------------------------------------------------------------------

def world_file(sections):
    """En-tête, répertoire (étiquette, position, taille), puis les sections bout à bout."""
    header = b"ODYW" + struct.pack("<HH", VERSION, len(sections))
    offset = len(header) + 20 * len(sections)
    directory = b""
    for tag, payload in sections:
        directory += tag + struct.pack("<QQ", offset, len(payload))
        offset += len(payload)
    return header + directory + b"".join(payload for _, payload in sections)


def main():
    layout = Layout.load(HERE / "world_layout.json")
    meta = json.loads((OUTPUT / "terrain_meta.json").read_text(encoding="utf-8"))
    plan = blockout_plan.blockout_plan(layout, Terrain(OUTPUT), meta)
    raw = np.asarray(Image.open(OUTPUT / meta["heightmap"]["file"])).astype(np.uint16)
    exact = lambda x, y: (float(raw[int(round(y)), int(round(x))]) - 32768) / 64  # noqa: E731

    quads = hole_quads(plan.holes, raw.shape)
    boxes = collision_boxes(plan)
    found = markers(layout, plan, meta, exact)
    data = world_file([(b"TERR", terrain_section(raw)), (b"HOLE", holes_section(quads)),
                       (b"BOXS", boxes_section(boxes)), (b"MARK", markers_section(found))])
    (OUTPUT / "odyssee.world").write_bytes(data)
    print(f"Fichier monde : {raw.shape[1] - 1} x {raw.shape[0] - 1} cases, {len(quads)} cases "
          f"percées, {len(boxes)} blocs, {len(found)} marqueurs ; "
          f"{len(data) / 1e6:.1f} Mo dans {OUTPUT / 'odyssee.world'}")


if __name__ == "__main__":
    main()
