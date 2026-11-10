#!/usr/bin/env python3
"""Génère le relief d'Odyssée à partir de world_layout.json.

Sorties (dans tools/worldgen/output/) :
  heightmap.png      16 bits, 4033 x 3025 : à importer dans Unreal (Landscape > Import from file)
  heights_4m.bin     hauteurs tous les 4 m (uint16 little-endian) : lues par le script Unreal
  terrain_meta.json  dimensions, échelle et encodage des hauteurs
  preview.png        aperçu en couleur (zones, eau, relief, lieux) pour vérifier sans Unreal

Encodage des hauteurs (identique pour les deux fichiers) : valeur = 32768 + hauteur_m * 64.
Dans Unreal : Landscape à l'origine, échelle (100, 100, 200) => 1 m par point, ±512 m.

Usage : python3 tools/worldgen/generate_terrain.py   (dépendances : numpy, Pillow)
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
LAYOUT = HERE / "world_layout.json"
OUTPUT = HERE / "output"
SEED = 2026
HEIGHT_SCALE = 64  # unités de heightmap par mètre (échelle Z Unreal = 200)
WORK_SCALE = 4     # les masques sont calculés à 1/4 de résolution, puis agrandis

# Profils de terrain : hauteur de base (m) et amplitude du bruit (m).
PROFILES = {
    "plaine": (10.0, 6.0),
    "collines": (16.0, 14.0),
    "ville": (7.0, 0.8),
    "marais": (0.4, 2.6),   # légèrement sous l'eau par endroits : des mares
    "foret_coeur": (24.0, 20.0),
}


# --- Outils de base ---------------------------------------------------------------------------

def load_layout():
    with LAYOUT.open(encoding="utf-8") as f:
        return json.load(f)


def grid_shape(layout):
    """Nombre de points (lignes, colonnes) : un point par mètre, bords inclus."""
    world = layout["world"]
    return world["height_m"] + 1, world["width_m"] + 1


def resize(array, shape):
    """Agrandit ou réduit un tableau float32 en interpolation bicubique."""
    image = Image.fromarray(array.astype(np.float32), mode="F")
    return np.asarray(image.resize((shape[1], shape[0]), Image.BICUBIC), dtype=np.float32)


def _box_blur(array, radius_px, axis):
    """Moyenne glissante sur 2 * radius_px + 1 points le long d'un axe (sommes cumulées)."""
    pad = [(0, 0), (0, 0)]
    pad[axis] = (radius_px + 1, radius_px)
    sums = np.cumsum(np.pad(array, pad, mode="edge"), axis=axis, dtype=np.float64)
    width = 2 * radius_px + 1
    upper = np.take(sums, range(width, sums.shape[axis]), axis=axis)
    lower = np.take(sums, range(0, sums.shape[axis] - width), axis=axis)
    return ((upper - lower) / width).astype(np.float32)


def gaussian_blur(array, sigma_px):
    """Flou gaussien approché par trois flous « boîte » successifs (rapide, sans dépendance)."""
    if sigma_px < 0.5:
        return array
    radius = max(1, round((np.sqrt(4 * sigma_px * sigma_px + 1) - 1) / 2))
    for _ in range(3):
        array = _box_blur(_box_blur(array, radius, 0), radius, 1)
    return array


def blur(array, radius_m):
    """Flou gaussien (rayon en mètres), calculé à basse résolution pour la vitesse."""
    small_shape = (array.shape[0] // WORK_SCALE, array.shape[1] // WORK_SCALE)
    blurred = gaussian_blur(resize(array, small_shape), radius_m / WORK_SCALE)
    return resize(blurred, array.shape)


def lerp(a, b, t):
    return a + (b - a) * t


# --- Masques (formes dessinées) ---------------------------------------------------------------

def blank_mask(shape):
    image = Image.new("F", (shape[1] // WORK_SCALE, shape[0] // WORK_SCALE), 0.0)
    return image, ImageDraw.Draw(image)


def finish_mask(image, shape, softness_m):
    """Adoucit les bords d'un masque dessiné, puis le ramène à pleine résolution (0 à 1).

    Les formes sont dessinées agrandies de softness_m / 2 et floutées d'autant : l'intérieur reste
    à 1 (effet complet) et le bord descend vers 0 sur environ softness_m mètres.
    """
    small = gaussian_blur(np.asarray(image, dtype=np.float32), softness_m / 2 / WORK_SCALE)
    return np.clip(resize(small, shape), 0.0, 1.0)


def scaled(points):
    return [(x / WORK_SCALE, y / WORK_SCALE) for x, y in points]


def polygon_mask(shape, polygon, softness_m):
    image, draw = blank_mask(shape)
    draw.polygon(scaled(polygon), fill=1.0)
    return finish_mask(image, shape, softness_m)


def polyline_mask(shape, points, width_m, softness_m):
    image, draw = blank_mask(shape)
    width = max(1, round((width_m + softness_m) / WORK_SCALE))
    draw.line(scaled(points), fill=1.0, width=width, joint="curve")
    return finish_mask(image, shape, softness_m)


def disk_mask(shape, center, radius_m, softness_m):
    image, draw = blank_mask(shape)
    x, y = center[0] / WORK_SCALE, center[1] / WORK_SCALE
    r = (radius_m + softness_m / 2) / WORK_SCALE
    draw.ellipse((x - r, y - r, x + r, y + r), fill=1.0)
    return finish_mask(image, shape, softness_m)


# --- Bruit ------------------------------------------------------------------------------------

def fractal_noise(shape, rng, base_cell_m, octaves):
    """Bruit fractal entre -1 et 1 : des grilles aléatoires de plus en plus fines, superposées."""
    total = np.zeros(shape, dtype=np.float32)
    amplitude, weight, cell = 1.0, 0.0, base_cell_m
    for _ in range(octaves):
        coarse = rng.uniform(-1, 1, (shape[0] // cell + 3, shape[1] // cell + 3))
        total += amplitude * resize(coarse, shape)
        weight += amplitude
        amplitude *= 0.5
        cell = max(4, cell // 2)
    return total / weight


def ridged(noise):
    """Crêtes de montagne : les zéros du bruit deviennent des arêtes vives."""
    return (1.0 - np.abs(noise)) ** 2


# --- Construction du relief -------------------------------------------------------------------

def profile_height(terrain, noise, mountain_noise):
    if terrain == "montagne":
        # Crêtes larges plutôt qu'aiguilles : la zone doit rester explorable (pentes < ~45°).
        return 70.0 + 230.0 * ridged(mountain_noise) ** 1.6
    base, amplitude = PROFILES[terrain]
    return base + amplitude * noise


def blend_zones(layout, shape, noise, mountain_noise):
    """Mélange les profils des zones : pas de marche brutale aux frontières."""
    weighted = np.zeros(shape, dtype=np.float32)
    weights = np.full(shape, 1e-6, dtype=np.float32)
    for zone in layout["zones"].values():
        softness = 220 if zone["terrain"] == "montagne" else 120
        mask = polygon_mask(shape, zone["polygon"], softness)
        weighted += mask * profile_height(zone["terrain"], noise, mountain_noise)
        weights += mask
    return weighted / weights


def apply_areas(layout, shape, height, noise, mountain_noise):
    """Sous-régions (ex. Bois de la lisière) : leur profil remplace celui de la zone."""
    for area in layout["areas"].values():
        mask = polygon_mask(shape, area["polygon"], 150)
        height = lerp(height, profile_height(area["terrain"], noise, mountain_noise), mask)
    return height


def carve_water(layout, shape, height, noise):
    """Vallée et lit de la rivière, cuvette du lac : l'eau affleure au niveau 0."""
    river = layout["water"]["river"]
    valley = polyline_mask(shape, river["path"], river["width_m"] * 6, 90)
    height = lerp(height, 3.0 + 1.5 * noise, valley)
    channel = polyline_mask(shape, river["path"], river["width_m"], 8)
    height = lerp(height, -river["depth_m"], channel)

    for lake in layout["water"]["lakes"]:
        shore = disk_mask(shape, lake["center"], lake["radius_m"] * 1.8, 80)
        height = lerp(height, 2.0 + noise, shore)
        bed = disk_mask(shape, lake["center"], lake["radius_m"], 15)
        height = lerp(height, -4.0, bed)
    return height


def carve_mountain_pass(layout, shape, height):
    """Le col du plateau : une montée praticable de la ville jusqu'à la Grande Porte."""
    col = next(p for p in layout["paths"] if p["id"] == "col_du_plateau")
    porte = next(p for p in layout["places"] if p["id"] == "grande_porte")
    start, end = 9.0, float(porte["height_m"])

    # On dessine le chemin en « peignant » sa hauteur, qui monte d'un segment à l'autre…
    image, draw = blank_mask(shape)
    points = scaled(col["points"])
    for i in range(len(points) - 1):
        level = lerp(start, end, (i + 0.5) / (len(points) - 1))
        draw.line([points[i], points[i + 1]], fill=level, width=round(60 / WORK_SCALE))
    painted = resize(np.asarray(image, dtype=np.float32), shape)

    # …puis on lisse cette hauteur peinte pour obtenir une pente continue.
    covered = (painted > 0).astype(np.float32)
    target = blur(painted, 40) / np.maximum(blur(covered, 40), 1e-3)
    corridor = polyline_mask(shape, col["points"], 60, 40)
    return lerp(height, target, corridor)


def flatten_places(layout, shape, height):
    """Aplanit le sol des lieux (bâtiments, villages) à leur hauteur locale ou imposée."""
    for place in layout["places"]:
        mask = disk_mask(shape, place["pos"], place["radius_m"], place["radius_m"] * 0.6)
        if "height_m" in place:
            level = float(place["height_m"])
        else:
            x, y = place["pos"]
            level = float(blur(height, place["radius_m"])[int(y), int(x)])
        height = lerp(height, np.float32(max(level, 1.5)), mask)
    return height


def smooth_paths(layout, shape, height):
    """Adoucit le terrain le long des routes et chemins (sauf le col, déjà sculpté)."""
    smoothed = blur(height, 30)
    for path in layout["paths"]:
        if path["id"] == "col_du_plateau":
            continue
        mask = polyline_mask(shape, path["points"], path["width_m"] * 2, path["width_m"])
        height = lerp(height, np.maximum(smoothed, 1.5), mask * 0.85)
    return height


def raise_world_border(shape, height):
    """Relève les bords du monde : on ne peut pas tomber dans le vide."""
    rows, cols = np.indices(shape, dtype=np.float32)
    edge = np.minimum.reduce([rows, cols, shape[0] - 1 - rows, shape[1] - 1 - cols])
    border = np.clip(1.0 - edge / 160.0, 0.0, 1.0) ** 2
    return height + border * 45.0


def build_height(layout):
    shape = grid_shape(layout)
    rng = np.random.default_rng(SEED)
    noise = fractal_noise(shape, rng, base_cell_m=400, octaves=6)
    mountain_noise = fractal_noise(shape, rng, base_cell_m=600, octaves=4)

    height = blend_zones(layout, shape, noise, mountain_noise)
    height = apply_areas(layout, shape, height, noise, mountain_noise)
    height = carve_water(layout, shape, height, noise)
    height = carve_mountain_pass(layout, shape, height)
    height = smooth_paths(layout, shape, height)
    height = flatten_places(layout, shape, height)
    height = raise_world_border(shape, height)
    return np.clip(height, -100.0, 500.0)


# --- Écriture des fichiers --------------------------------------------------------------------

def encode(height):
    return np.clip(32768 + np.round(height * HEIGHT_SCALE), 0, 65535).astype(np.uint16)


def write_heightmap(height):
    Image.fromarray(encode(height), mode="I;16").save(OUTPUT / "heightmap.png")


def write_coarse_heights(height, step_m=4):
    coarse = encode(height[::step_m, ::step_m])
    coarse.astype("<u2").tofile(OUTPUT / "heights_4m.bin")
    meta = {
        "heightmap": {"file": "heightmap.png", "width": height.shape[1], "height": height.shape[0]},
        "coarse": {"file": "heights_4m.bin", "width": coarse.shape[1], "height": coarse.shape[0],
                   "step_m": step_m, "dtype": "uint16-le", "row_major": True},
        "encoding": "hauteur_m = (valeur - 32768) / 64",
        "unreal_landscape": {"location_cm": [0, 0, 0], "scale": [100, 100, 200]},
        "seed": SEED,
    }
    (OUTPUT / "terrain_meta.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    return coarse


def main():
    OUTPUT.mkdir(exist_ok=True)
    layout = load_layout()
    height = build_height(layout)
    write_heightmap(height)
    write_coarse_heights(height)

    from preview import write_preview  # aperçu dans son propre fichier (une notion par fichier)
    write_preview(layout, height, OUTPUT / "preview.png")

    print(f"Relief : {height.shape[1]} x {height.shape[0]} points, "
          f"de {height.min():.1f} m à {height.max():.1f} m ; fichiers dans {OUTPUT}")


if __name__ == "__main__":
    main()
