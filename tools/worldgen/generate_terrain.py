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
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
LAYOUT = HERE / "world_layout.json"
OUTPUT = HERE / "output"
SEED = 2026

# Les modules « purs » de odyssee_world (géométrie du plan) servent aussi ici.
sys.path.insert(0, str(HERE / "unreal"))
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


def _cave_frame(height, cave, reach_m):
    """Fenêtre autour d'une grotte et coordonnées locales de chaque point :
    a = distance vers l'extérieur (sens de l'ouverture), b = distance latérale."""
    x0, y0 = cave["pos"]
    rows = slice(max(0, int(y0 - reach_m)), min(height.shape[0], int(y0 + reach_m) + 1))
    cols = slice(max(0, int(x0 - reach_m)), min(height.shape[1], int(x0 + reach_m) + 1))
    ys, xs = np.mgrid[rows, cols].astype(np.float32)
    yaw = np.radians(cave["yaw_deg"])
    ux, uy = np.cos(yaw), np.sin(yaw)
    dx, dy = xs - x0, ys - y0
    return (rows, cols), dx * ux + dy * uy, -dx * uy + dy * ux


def carve_caves(layout, height):
    """Grottes de montagne : un replat devant l'entrée, à la hauteur du sol de la grotte.

    Le tunnel et la salle ne sont PAS creusés ici : une heightmap n'a pas de plafond (creuser
    enlèverait aussi la montagne au-dessus). Ils sont construits en roche sous le relief dans
    Unreal, et le Landscape est percé (couche de visibilité) là où le tunnel sort de la pente.
    Renvoie aussi la hauteur du sol de chaque grotte ({id: m}).
    """
    shapes = layout["cave_shapes"]
    wall, apron = shapes["wall_m"], shapes["apron_m"]
    half_tunnel = shapes["montagne"]["tunnel"][1] / 2 + wall
    floors = {}
    for cave in layout.get("caves", []):
        if cave["type"] != "montagne":
            continue
        window, a, b = _cave_frame(height, cave, apron + 10)
        sub = height[window]
        floor = float(sub[(np.abs(a) <= 2) & (np.abs(b) <= 2)].mean())

        ahead = np.clip(1.0 - (a - 1.0) / apron, 0.0, 1.0) * (a > -0.5)
        beside = np.clip(1.0 - (np.abs(b) - half_tunnel - 1.0) / 4.0, 0.0, 1.0)
        height[window] = lerp(sub, np.float32(floor), ahead * beside)
        floors[cave["id"]] = round(floor, 3)
    return height, floors


def raise_cave_cliffs(layout, height, floors):
    """Au-dessus de l'entrée de chaque grotte de montagne, une masse de roche à flanc vertical :
    l'entrée s'ouvre dans une petite falaise, et le tunnel passe aussitôt sous le relief (le trou
    à percer dans le Landscape se réduit à la face de la falaise)."""
    shapes = layout["cave_shapes"]
    tunnel, wall = shapes["montagne"]["tunnel"], shapes["wall_m"]
    half = tunnel[1] / 2 + wall + 3.0
    for cave in layout.get("caves", []):
        if cave["type"] != "montagne":
            continue
        window, a, b = _cave_frame(height, cave, 30)
        top = np.float32(floors[cave["id"]] + tunnel[2] + 2.5)
        # Face nette en a = 0, masse pleine sur 8 m, puis raccord progressif à la pente.
        along = np.clip((a + 16.0) / 8.0, 0.0, 1.0) * (a <= 0.0)
        across = np.clip((half + 4.0 - np.abs(b)) / 4.0, 0.0, 1.0)
        sub = height[window]
        height[window] = np.maximum(sub, lerp(sub, top, along * across))
    return height


# --- Relief détaillé (world_layout.json, « relief ») -------------------------------------------

def _terrain_amplitude_map(layout, shape, amplitudes):
    """Pour chaque point, l'amplitude voulue selon le type de terrain de sa zone (mélangée aux
    frontières comme les profils)."""
    weighted = np.zeros(shape, dtype=np.float32)
    weights = np.full(shape, 1e-6, dtype=np.float32)
    regions = list(layout["zones"].values()) + list(layout["areas"].values())
    for region in regions:
        mask = polygon_mask(shape, region["polygon"], 150)
        weighted += mask * np.float32(amplitudes.get(region["terrain"], 0.0))
        weights += mask
    return weighted / weights


def add_hills(layout, shape, height):
    """Collines douces : surtout dans la plaine de départ, qui sinon paraît plate."""
    hills = layout["relief"]["hills"]
    rng = np.random.default_rng(SEED + 1)
    noise = fractal_noise(shape, rng, base_cell_m=hills["cell_m"], octaves=3)
    return height + _terrain_amplitude_map(layout, shape, hills["amplitude_m"]) * noise


def carve_ravines(layout, shape, height):
    """Ravins en deux gradins (bords évasés, fond étroit) ; le fond reste au-dessus de l'eau."""
    for ravine in layout["relief"]["ravines"]:
        width, depth = ravine["width_m"], ravine["depth_m"]
        outer = polyline_mask(shape, ravine["points"], width * 3, width)
        inner = polyline_mask(shape, ravine["points"], width, 6)
        carved = height - depth * (0.45 * outer + 0.55 * inner)
        height = np.where(outer > 0, np.maximum(carved, np.minimum(height, 1.0)), height)
    return height


def _outline(rng, sides):
    """Rayon relatif selon l'angle, irrégulier : une forme de rocher, pas un disque."""
    phases = rng.uniform(0, 2 * np.pi, 3)
    weights = rng.uniform(0.06, 0.16, 3)
    return lambda angle: 1.0 + sum(w * np.sin((k + 2) * angle + p)
                                   for k, (w, p) in enumerate(zip(weights, phases)))


def _raise_shape(height, center, radius, rise, profile, outline):
    """Ajoute `rise` × profile(distance relative) dans une fenêtre autour de `center`."""
    x0, y0 = center
    reach = radius * 1.4
    rows = slice(max(0, int(y0 - reach)), min(height.shape[0], int(y0 + reach) + 1))
    cols = slice(max(0, int(x0 - reach)), min(height.shape[1], int(x0 + reach) + 1))
    ys, xs = np.mgrid[rows, cols].astype(np.float32)
    dx, dy = xs - x0, ys - y0
    relative = np.hypot(dx, dy) / (radius * outline(np.arctan2(dy, dx)))
    height[rows, cols] += np.float32(rise) * profile(relative)


def raise_buttes(layout, height):
    """Buttes à deux étages : un socle large et un sommet plus petit, flancs raides."""
    rng = np.random.default_rng(SEED + 2)
    tiers = lambda r: (0.55 * np.clip((1.0 - r) / 0.15, 0, 1)
                       + 0.45 * np.clip((0.72 - r) / 0.12, 0, 1))
    for butte in layout["relief"]["buttes"]:
        _raise_shape(height, butte["pos"], butte["radius_m"], butte["height_m"], tiers,
                     _outline(rng, 6))
    return height


def _outcrop_spots(layout, rules, rng):
    """Emplacements des rochers : tirés zone par zone, loin de tout ce qui doit rester dégagé."""
    from odyssee_world.layout import Layout, bounds, point_in_polygon  # module pur, sans Unreal

    geo = Layout(layout)
    spawn = next(p["pos"] for p in layout["places"] if p.get("spawn"))
    caves = [c["pos"] for c in layout.get("caves", [])]

    def clear(x, y):
        return (geo.distance_to_paths(x, y) > 25 and geo.distance_to_river(x, y) > 60
                and not geo.inside_place(x, y, 40)
                and all(np.hypot(x - c[0], y - c[1]) > 60 for c in caves)
                and np.hypot(x - spawn[0], y - spawn[1]) > rules["spawn_clearance_m"]
                and all(np.hypot(x - l["center"][0], y - l["center"][1]) > l["radius_m"] * 1.8
                        for l in layout["water"]["lakes"]))

    for zone in layout["zones"].values():
        polygon = zone["polygon"]
        x_min, y_min, x_max, y_max = bounds(polygon)
        area_km2 = (x_max - x_min) * (y_max - y_min) / 1e6
        for _ in range(int(area_km2 * rules["per_km2"].get(zone["terrain"], 0))):
            x, y = rng.uniform(x_min, x_max), rng.uniform(y_min, y_max)
            if point_in_polygon(x, y, polygon) and clear(x, y):
                yield x, y


def scatter_outcrops(layout, height):
    """Rochers affleurants : flancs raides et sommet plat, pour accrocher la lumière en facettes."""
    rules = layout["relief"]["outcrops"]
    rng = np.random.default_rng(rules["seed"])
    rock = lambda r: np.clip((1.0 - r) / 0.3, 0, 1)
    count = 0
    for x, y in list(_outcrop_spots(layout, rules, rng)):
        radius = rng.uniform(*rules["radius_m"])
        rise = rng.uniform(*rules["height_m"])
        _raise_shape(height, (x, y), radius, rise, rock, _outline(rng, 5))
        count += 1
    return height, count


def _calm_mask(layout, shape):
    """1 là où le sol doit rester net (chemins, lieux, grottes, eau), 0 ailleurs."""
    calm = np.zeros(shape, dtype=np.float32)
    for path in layout["paths"]:
        calm = np.maximum(calm, polyline_mask(shape, path["points"], path["width_m"] * 2, 12))
    for place in layout["places"]:
        calm = np.maximum(calm, disk_mask(shape, place["pos"], place["radius_m"] + 10, 12))
    for cave in layout.get("caves", []):
        calm = np.maximum(calm, disk_mask(shape, cave["pos"], 45, 15))
    river = layout["water"]["river"]
    calm = np.maximum(calm, polyline_mask(shape, river["path"], river["width_m"] * 2, 12))
    for lake in layout["water"]["lakes"]:
        calm = np.maximum(calm, disk_mask(shape, lake["center"], lake["radius_m"] * 1.3, 15))
    return calm


def facet(layout, height, step, jitter_m):
    """Style low-poly : le relief devient des triangles plans de `step` mètres (chaque carré de
    la grille grossière coupé en deux). Chaque sommet est décalé au hasard de ± jitter_m, pour
    que deux faces voisines n'aient jamais tout à fait la même pente et accrochent la lumière
    différemment. Chemins, lieux et eau ne sont pas décalés ; autour des entrées de grotte, le
    relief fin est gardé tel quel."""
    calm = _calm_mask(layout, height.shape)
    rng = np.random.default_rng(SEED + 3)
    coarse = height[::step, ::step].copy()
    coarse += rng.uniform(-jitter_m, jitter_m, coarse.shape).astype(np.float32) * (
        1.0 - calm[::step, ::step])
    rows, cols = np.indices(height.shape, dtype=np.float32)
    gy, gx = rows / step, cols / step
    i = np.minimum(gy.astype(np.int32), coarse.shape[0] - 2)
    j = np.minimum(gx.astype(np.int32), coarse.shape[1] - 2)
    fy, fx = gy - i, gx - j
    h00, h10 = coarse[i, j], coarse[i, j + 1]
    h01, h11 = coarse[i + 1, j], coarse[i + 1, j + 1]
    lower = fx + fy <= 1.0
    faceted = np.where(lower, h00 + fx * (h10 - h00) + fy * (h01 - h00),
                       h11 + (1 - fx) * (h01 - h11) + (1 - fy) * (h10 - h11))

    keep = np.zeros(height.shape, dtype=np.float32)
    for cave in layout.get("caves", []):
        if cave["type"] == "montagne":
            x, y = cave["pos"]
            keep = np.maximum(keep, np.clip(1.0 - (np.hypot(cols - x, rows - y) - 30) / 15, 0, 1))
    return lerp(faceted.astype(np.float32), height, keep)


def build_height(layout):
    """Relief complet (m) et hauteur du sol de chaque grotte de montagne ({id: m})."""
    shape = grid_shape(layout)
    rng = np.random.default_rng(SEED)
    noise = fractal_noise(shape, rng, base_cell_m=400, octaves=6)
    mountain_noise = fractal_noise(shape, rng, base_cell_m=600, octaves=4)

    height = blend_zones(layout, shape, noise, mountain_noise)
    height = apply_areas(layout, shape, height, noise, mountain_noise)
    height = add_hills(layout, shape, height)
    height = raise_buttes(layout, height)
    height, _ = scatter_outcrops(layout, height)
    height = carve_ravines(layout, shape, height)
    height = carve_water(layout, shape, height, noise)
    height = carve_mountain_pass(layout, shape, height)
    height = smooth_paths(layout, shape, height)
    height = flatten_places(layout, shape, height)
    height = raise_world_border(shape, height)
    height, cave_floors = carve_caves(layout, height)
    height = raise_cave_cliffs(layout, height, cave_floors)
    relief = layout["relief"]
    height = facet(layout, height, relief["facet_m"], relief["facet_jitter_m"])
    return np.clip(height, -100.0, 500.0), cave_floors


# --- Écriture des fichiers --------------------------------------------------------------------

def encode(height):
    return np.clip(32768 + np.round(height * HEIGHT_SCALE), 0, 65535).astype(np.uint16)


def write_heightmap(height):
    # uint16 → image 16 bits « I;16 » (Pillow 13 refuse le paramètre mode pour ce type).
    Image.fromarray(encode(height)).save(OUTPUT / "heightmap.png")


def write_coarse_heights(height, cave_floors, step_m=4):
    coarse = encode(height[::step_m, ::step_m])
    coarse.astype("<u2").tofile(OUTPUT / "heights_4m.bin")
    meta = {
        "heightmap": {"file": "heightmap.png", "width": height.shape[1], "height": height.shape[0]},
        "coarse": {"file": "heights_4m.bin", "width": coarse.shape[1], "height": coarse.shape[0],
                   "step_m": step_m, "dtype": "uint16-le", "row_major": True},
        "encoding": "hauteur_m = (valeur - 32768) / 64",
        "unreal_landscape": {"location_cm": [0, 0, 0], "scale": [100, 100, 200]},
        "seed": SEED,
        # Sol des grottes creusées : trop étroites pour la grille à 4 m, on le donne exactement.
        "cave_floors_m": cave_floors,
    }
    (OUTPUT / "terrain_meta.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    return coarse


def main():
    OUTPUT.mkdir(exist_ok=True)
    layout = load_layout()
    height, cave_floors = build_height(layout)
    write_heightmap(height)
    write_coarse_heights(height, cave_floors)

    from preview import write_preview  # aperçu dans son propre fichier (une notion par fichier)
    write_preview(layout, height, OUTPUT / "preview.png")

    print(f"Relief : {height.shape[1]} x {height.shape[0]} points, "
          f"de {height.min():.1f} m à {height.max():.1f} m ; fichiers dans {OUTPUT}")


if __name__ == "__main__":
    main()
