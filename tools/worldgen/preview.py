"""Aperçu en couleur du relief et du plan du monde, pour vérifier sans ouvrir Unreal."""
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

SCALE = 4  # 1 pixel de l'aperçu = 4 m

ZONE_COLORS = {  # teintes proches de la carte de référence
    "plaine": (150, 190, 95), "collines": (95, 150, 70), "ville": (150, 150, 140),
    "marais": (120, 110, 140), "montagne": (225, 230, 240), "foret_coeur": (45, 105, 60),
}
WATER = (70, 130, 190)
ICE = (190, 225, 240)
FONT_FILE = Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")


def _font():
    """Police avec accents si disponible, sinon celle de Pillow."""
    if FONT_FILE.exists():
        return ImageFont.truetype(str(FONT_FILE), 11)
    return ImageFont.load_default()


def _zone_colors(layout, shape):
    """Image couleur où chaque zone a sa teinte (les sous-régions par-dessus)."""
    image = Image.new("RGB", (shape[1], shape[0]))
    draw = ImageDraw.Draw(image)
    regions = list(layout["zones"].values()) + list(layout["areas"].values())
    for region in regions:
        points = [(x / SCALE, y / SCALE) for x, y in region["polygon"]]
        draw.polygon(points, fill=ZONE_COLORS[region["terrain"]])
    return np.asarray(image, dtype=np.float32)


def _hillshade(height):
    """Ombrage du relief (lumière venant du nord-ouest)."""
    dy, dx = np.gradient(height)
    shade = (-dx - dy) / np.sqrt(dx * dx + dy * dy + 1.0)
    return np.clip(0.75 + 0.45 * shade, 0.35, 1.25)


def _base_image(layout, height):
    small = height[::SCALE, ::SCALE]
    colors = _zone_colors(layout, small.shape) * _hillshade(small)[..., None]
    colors[small < 0] = WATER
    for lake in layout["water"]["lakes"]:
        if lake.get("frozen"):
            cx, cy = lake["center"][0] / SCALE, lake["center"][1] / SCALE
            r = lake["radius_m"] / SCALE
            rows, cols = np.indices(small.shape)
            inside = ((cols - cx) ** 2 + (rows - cy) ** 2 < r * r) & (small < 0)
            colors[inside] = ICE
    return Image.fromarray(np.clip(colors, 0, 255).astype(np.uint8))


def _point(pos):
    return pos[0] / SCALE, pos[1] / SCALE


def _marker(draw, pos, color, radius=4):
    x, y = _point(pos)
    draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=color, outline=(0, 0, 0))


def _draw_overlays(layout, image):
    draw = ImageDraw.Draw(image)
    font = _font()
    for level in layout["toxic_fog"]["levels"]:
        x, y = _point(layout["toxic_fog"]["center"])
        r = level["radius_m"] / SCALE
        draw.ellipse((x - r, y - r, x + r, y + r), outline=(140, 80, 170))
    for path in layout["paths"]:
        color = (110, 90, 70) if path["kind"] != "rails" else (60, 50, 50)
        draw.line([_point(p) for p in path["points"]], fill=color, width=2)
    for barrier in layout["barriers"]:
        draw.line([_point(p) for p in barrier["points"]], fill=(60, 35, 25), width=5)
    for place in layout["places"]:
        _marker(draw, place["pos"], (255, 255, 255), 3)
        x, y = _point(place["pos"])
        draw.text((x + 6, y - 7), place["name"], fill=(20, 20, 20), font=font)
    for cocoon in layout["cocoons"]:
        _marker(draw, cocoon, (120, 255, 120), 3)
    for boss in layout["bosses"]:
        _marker(draw, boss["pos"], (210, 30, 30), 5)
    for artifact in layout["artifacts"]:
        _marker(draw, artifact["pos"], (255, 200, 0), 4)


def write_preview(layout, height, path):
    image = _base_image(layout, height)
    _draw_overlays(layout, image)
    image.save(path)
