"""La Ville engloutie : un quadrillage de rues et des immeubles (blocs provisoires)."""
from .layout import bounds
from .noise import seeded_random
from .placement import Placement

BLOCK_M = 70          # une rue tous les 70 m
STREET_M = 14         # largeur des rues
MIN_SIZE_M = 12       # emprise d'un immeuble
MAX_SIZE_M = 26
COLLAPSED_SHARE = 0.2 # part des immeubles effondrés
GAP_M = 3             # écart minimal entre deux immeubles


def generate_buildings(layout, terrain, seed, zone_id="ville"):
    x0, y0, x1, y1 = bounds(layout.zones[zone_id]["polygon"])
    buildings = []
    for bx in range(int(x0 // BLOCK_M), int(x1 // BLOCK_M) + 1):
        for by in range(int(y0 // BLOCK_M), int(y1 // BLOCK_M) + 1):
            buildings += _buildings_in_block(bx, by, layout, terrain, seed, zone_id)
    return buildings


def _buildings_in_block(bx, by, layout, terrain, seed, zone_id):
    """Un pâté de maisons : 1 à 3 immeubles dans l'espace laissé par les rues."""
    rng = seeded_random(seed, "ville", bx, by)
    inner = BLOCK_M - STREET_M
    left, top = bx * BLOCK_M + STREET_M / 2, by * BLOCK_M + STREET_M / 2
    result = []
    for _ in range(rng.randint(1, 3)):
        width, depth = rng.uniform(MIN_SIZE_M, MAX_SIZE_M), rng.uniform(MIN_SIZE_M, MAX_SIZE_M)
        x = left + rng.uniform(width / 2, inner - width / 2)
        y = top + rng.uniform(depth / 2, inner - depth / 2)
        if _overlaps(x, y, width, depth, result):
            continue
        if _fits(x, y, max(width, depth), layout, terrain, zone_id):
            result.append(Placement("immeuble", x, y, _ground(x, y, width, depth, terrain), 0.0,
                                    (width, depth, _height(rng))))
    return result


def _overlaps(x, y, width, depth, others):
    """Vrai si ce rectangle (agrandi de GAP_M) touche un immeuble déjà placé."""
    for other in others:
        ow, od = other.scale[0], other.scale[1]
        touches_x = abs(x - other.x) < (width + ow) / 2 + GAP_M
        touches_y = abs(y - other.y) < (depth + od) / 2 + GAP_M
        if touches_x and touches_y:
            return True
    return False


def _fits(x, y, size, layout, terrain, zone_id):
    if layout.zone_at(x, y) != zone_id:
        return False
    if terrain.height(x, y) < 0.8 or terrain.slope_deg(x, y) > 12:
        return False
    if layout.distance_to_river(x, y) < size or layout.distance_to_paths(x, y) < size / 2:
        return False
    return not layout.inside_place(x, y, size)


def _ground(x, y, width, depth, terrain):
    """Le bloc repose sur le point le plus bas de son emprise (pas de vide dessous)."""
    corners = [(x + dx * width / 2, y + dy * depth / 2) for dx in (-1, 1) for dy in (-1, 1)]
    return min(terrain.height(cx, cy) for cx, cy in corners)


def _height(rng):
    if rng.random() < COLLAPSED_SHARE:
        return rng.uniform(3, 6)   # ruine effondrée
    return rng.uniform(8, 34)
