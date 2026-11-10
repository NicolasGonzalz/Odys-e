"""Dispersion naturelle de la végétation et des détails, zone par zone (scatter_rules.json)."""
import math

from .layout import bounds
from .noise import seeded_random, value_noise
from .placement import Placement

PATH_CLEARANCE_M = 1.5    # rien sur les routes et chemins
RIVER_CLEARANCE_M = 2.0   # rien dans la rivière (sauf les roseaux)
PLACE_MARGIN_M = 5.0      # l'emprise des lieux reste dégagée
WATER_MARGIN_M = 0.4      # hauteur minimale par défaut : hors de l'eau
BUSH_CELL_M = 120         # taille des bosquets / clairières


def scatter_region(region_id, rules, layout, terrain, catalog, seed):
    """Toutes les règles d'une zone (ou sous-région)."""
    placements = []
    for rule in rules:
        placements += scatter_rule(region_id, rule, layout, terrain, catalog, seed)
    return placements


def scatter_rule(region_id, rule, layout, terrain, catalog, seed):
    """Une règle : une grille dont chaque case reçoit au plus un élément, décalé au hasard."""
    cell = math.sqrt(10_000 / rule["per_ha"])
    x0, y0, x1, y1 = bounds(layout.region_polygon(region_id))
    element = catalog[rule["element"]]
    placements = []

    for ix in range(int((x1 - x0) / cell) + 1):
        for iy in range(int((y1 - y0) / cell) + 1):
            rng = seeded_random(seed, region_id, rule["element"], ix, iy)
            x = x0 + (ix + rng.random()) * cell
            y = y0 + (iy + rng.random()) * cell
            if rng.random() > _keep_probability(x, y, rule, seed):
                continue
            if not _accepts(x, y, region_id, rule, layout, terrain):
                continue
            placements.append(_place(element, x, y, terrain, rng))
    return placements


def scatter_barrier(barrier, layout, terrain, catalog, seed, element_name="ronce"):
    """Muraille de ronces : plusieurs rangées serrées le long d'une ligne."""
    element = catalog[element_name]
    placements = []
    points = barrier["points"]
    for index, (a, b) in enumerate(zip(points, points[1:])):
        length = math.dist(a, b)
        nx, ny = -(b[1] - a[1]) / length, (b[0] - a[0]) / length  # normale au segment
        for step in range(int(length / 4)):
            for row in (-1, 0, 1):
                rng = seeded_random(seed, barrier["id"], index, step, row)
                t = (step + rng.random()) * 4 / length
                offset = row * barrier["width_m"] / 3 + rng.uniform(-2, 2)
                x = a[0] + (b[0] - a[0]) * t + nx * offset
                y = a[1] + (b[1] - a[1]) * t + ny * offset
                placements.append(_place(element, x, y, terrain, rng))
    return placements


def _keep_probability(x, y, rule, seed):
    """Bosquets et clairières : la densité varie lentement d'un endroit à l'autre."""
    cluster = rule.get("cluster", 0.0)
    noise = value_noise(x, y, BUSH_CELL_M, (seed, rule["element"]))
    grove = min(1.0, max(0.0, (noise - 0.3) / 0.4))
    return (1.0 - cluster) + cluster * grove


def _accepts(x, y, region_id, rule, layout, terrain):
    """Le point convient-il ? (zone, altitude, pente, chemins, rivière, lieux)"""
    if layout.region_at(x, y) != region_id:
        return False
    height = terrain.height(x, y)
    if not rule.get("min_height", WATER_MARGIN_M) <= height <= rule.get("max_height", 1e9):
        return False
    if terrain.slope_deg(x, y) > rule.get("max_slope", 30):
        return False
    if layout.distance_to_paths(x, y) < PATH_CLEARANCE_M:
        return False
    near_water_plant = rule["element"] == "roseaux"
    if not near_water_plant and layout.distance_to_river(x, y) < RIVER_CLEARANCE_M:
        return False
    return not layout.inside_place(x, y, PLACE_MARGIN_M)


def _place(element, x, y, terrain, rng):
    low, high = element.scale_range
    size = rng.uniform(low, high)
    return Placement(element.name, x, y, terrain.height(x, y), rng.uniform(0, 360),
                     (size, size, size))

