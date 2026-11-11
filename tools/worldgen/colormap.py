"""Carte des couleurs du sol (output/colormap.png), lue par le matériau du Landscape dans Unreal.

Une teinte par zone (fondue aux frontières), puis par-dessus : berges, lits de l'eau, chemins et
lieux. Le matériau y ajoute lui-même la roche sur les pentes raides et la neige en altitude, et
ombre chaque facette : ici, seulement des aplats (style low-poly).

1 pixel = 1 m : 4033 x 3025 pour le monde de 4032 x 3024 m (chemins de 4 à 8 m nets, même de près).
"""
import numpy as np
from PIL import Image

from generate_terrain import disk_mask, lerp, polygon_mask, polyline_mask

STEP_M = 1

ZONE_COLORS = {  # sRGB, palette douce et peu saturée
    "plaine": (118, 140, 84),
    "collines": (96, 124, 72),
    "ville": (128, 128, 112),
    "marais": (98, 106, 82),
    "montagne": (128, 124, 112),
    "foret_coeur": (64, 98, 72),
}
PATH_COLORS = {"route": (120, 112, 96), "chemin": (150, 124, 88), "rails": (104, 98, 92)}
SAND = (196, 180, 128)       # berges
WATER_BED = (92, 104, 96)    # fond de la rivière et du lac (sous l'eau, ou à sec en attendant)
PLACE_GROUND = (150, 136, 108)


def _color(rgb):
    return np.array(rgb, dtype=np.float32)


def _zone_layer(layout, shape):
    """Teinte des zones, mélangées sur ~60 m aux frontières."""
    weighted = np.zeros(shape + (3,), dtype=np.float32)
    weights = np.full(shape, 1e-6, dtype=np.float32)
    for region in list(layout["zones"].values()) + list(layout["areas"].values()):
        mask = polygon_mask(shape, region["polygon"], 60)
        weighted += mask[..., None] * _color(ZONE_COLORS[region["terrain"]])
        weights += mask
    return weighted / weights[..., None]


def _paint(colors, mask, rgb):
    return lerp(colors, _color(rgb), mask[..., None])


def _water_layers(layout, shape, height, colors):
    """Berges sableuses près de l'eau, fond de l'eau plus sombre."""
    near_water = np.clip((2.5 - height) / 1.5, 0.0, 1.0)
    river = layout["water"]["river"]
    valley = polyline_mask(shape, river["path"], river["width_m"] * 4, 30)
    for lake in layout["water"]["lakes"]:
        valley = np.maximum(valley, disk_mask(shape, lake["center"], lake["radius_m"] * 1.6, 30))
    colors = _paint(colors, near_water * valley, SAND)
    return _paint(colors, np.clip(-height / 1.0, 0.0, 1.0), WATER_BED)


def _fine_line(shape, points, width_m):
    """Masque d'un chemin à 1 m près (les masques du relief, à 4 m, le rendraient flou), calculé à
    partir de la distance exacte au tracé : bord net mais lissé sur un pixel (pas d'escalier)."""
    mask = np.zeros(shape, dtype=np.float32)
    half = width_m / 2
    for (ax, ay), (bx, by) in zip(points, points[1:]):
        x0, x1 = int(max(0, min(ax, bx) - half - 2)), int(min(shape[1], max(ax, bx) + half + 3))
        y0, y1 = int(max(0, min(ay, by) - half - 2)), int(min(shape[0], max(ay, by) + half + 3))
        ys, xs = np.mgrid[y0:y1, x0:x1].astype(np.float32)
        dx, dy = bx - ax, by - ay
        along = np.clip(((xs - ax) * dx + (ys - ay) * dy) / max(dx * dx + dy * dy, 1e-6), 0, 1)
        distance = np.hypot(xs - (ax + along * dx), ys - (ay + along * dy))
        coverage = np.clip(half - distance + 0.5, 0.0, 1.0)
        mask[y0:y1, x0:x1] = np.maximum(mask[y0:y1, x0:x1], coverage)
    return mask


def _path_layers(layout, shape, colors):
    for place in layout["places"]:
        ground = place.get("ground", place)  # sol nu du lieu (centre de stase : l'esplanade seule)
        colors = _paint(colors, disk_mask(shape, ground["pos"], ground["radius_m"], 8), PLACE_GROUND)
    for path in layout["paths"]:
        mask = _fine_line(shape, path["points"], path["width_m"])
        colors = _paint(colors, mask, PATH_COLORS.get(path["kind"], PATH_COLORS["chemin"]))
    return colors


def write_colormap(layout, height, path):
    """Calcule à pleine résolution (les chemins font quelques mètres), écrit à STEP_M m par pixel."""
    shape = height.shape
    colors = _zone_layer(layout, shape)
    colors = _water_layers(layout, shape, height, colors)
    colors = _path_layers(layout, shape, colors)
    small = colors[::STEP_M, ::STEP_M]
    Image.fromarray(np.clip(small, 0, 255).astype(np.uint8)).save(path)
