"""Planches (ressource transformée) : ce qu'on tire d'une bûche à l'établi. Abris, coffres, établis.

Pas de concept dédié : le bois de la bûche (lowpoly/wood.py), mais scié, donc plus clair (bois frais).
Une pile de 3 planches brutes, sciées à la main (bouts pas tout à fait d'équerre, un coin écorné),
légèrement décalées, tenues par deux tours de corde. Longueur 0,5 m.
"""
import random

from lowpoly.finish import turned
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.profile import profile_solid
from lowpoly.outline import without_outline
from lowpoly.shapes import cylinder, roughen
from lowpoly.wood import grain_strokes, streaked

ICON = True
VIEW = (-0.55, -1.0, 0.75)   # côté des bouts sciés
ICON_VIEW = VIEW

LENGTH = 0.5
WIDTH = 0.11
THICKNESS = 0.022
CORD_X = (0.12, 0.38)   # position des deux tours de corde le long des planches

WOOD = (0.70, 0.53, 0.36)        # bois frais scié : plus clair que les manches
WOOD_DARK = (0.55, 0.40, 0.27)
SAWN = (0.80, 0.65, 0.47)         # bouts sciés
GRAIN = (0.46, 0.33, 0.21)

KNOT = (0.36, 0.24, 0.15)
CORD = (0.62, 0.50, 0.33)

# Chaque planche : (décalage en x, décalage en y, rotation en degrés, teinte, graine).
# Trois planches ne sortent jamais tout à fait du même bois : teinte légèrement différente.
STACK = [(0.0, 0.0, 0.0, 1.0, 1), (0.012, -0.006, 3.0, 0.92, 2), (-0.008, 0.004, -2.0, 1.06, 3)]


def _outline(rng):
    """Contour d'une planche vue de dessus (x, largeur) : bouts sciés de travers, coin écorné."""
    skew = [rng.uniform(-0.008, 0.008) for _ in range(2)]
    w = WIDTH / 2
    return [(skew[0], -w), (LENGTH + skew[1], -w), (LENGTH - skew[1], w - 0.012),
            (LENGTH - skew[1] - 0.018, w), (-skew[0], w)]


def _tinted(color, tint):
    return tuple(min(1.0, c * tint) for c in color)


def _plank(name, tint, seed):
    """Une planche à plat : contour scié, tons en veines le long du fil, bouts clairs, fil et
    nœud sur le dessus."""
    rng = random.Random(seed)
    plank = profile_solid(name, _outline(rng), lambda _: THICKNESS,
                          [flat_material("planche_bois", WOOD)], lambda _: 0,
                          cuts_x=[LENGTH * i / 6 for i in range(1, 6)], chamfer=0.003)
    roughen(plank, 0.0006, seed)
    streaked(plank, _tinted(WOOD, tint), _tinted(WOOD_DARK, tint), f"planche_{seed}",
             axis=(1, 0, 0), dark_side=(0, 0, -1), threshold=0.3, streak=0.12, spread=0.12,
             seed=seed)
    _light_ends(plank)
    return [plank, *_grain(name, rng), _knot(name, rng)]


def _knot(name, rng):
    """Un nœud sombre, allongé dans le sens du fil, sur la face du dessus."""
    knot = cylinder(f"{name}_noeud", 0.008, 0.001, flat_material("planche_noeud", KNOT),
                    vertices=7, location=(rng.uniform(0.1, LENGTH - 0.1), THICKNESS / 2 + 0.0004,
                                          rng.uniform(-WIDTH * 0.3, WIDTH * 0.3)),
                    rotation_deg=(90, 0, 0))
    knot.scale = (1.7, 1.0, 1.0)
    return without_outline(knot)


def _light_ends(plank):
    """Les bouts sciés (faces tournées vers ±x) sont plus clairs que les faces."""
    plank.data.materials.append(flat_material("planche_bout", SAWN))
    end = len(plank.data.materials) - 1
    for polygon in plank.data.polygons:
        if abs(polygon.normal.x) > 0.8:
            polygon.material_index = end


def _grain(name, rng):
    """Traits du fil sur la face du dessus (y > 0 avant que la planche soit couchée)."""
    y = THICKNESS / 2 + 0.0003
    strokes = []
    for _ in range(4):
        z = rng.uniform(-WIDTH * 0.35, WIDTH * 0.35)
        x0 = rng.uniform(0.03, 0.15)
        x1 = rng.uniform(LENGTH - 0.15, LENGTH - 0.04)
        bend = rng.uniform(-0.006, 0.006)
        strokes.append([(x0, y, z), ((x0 + x1) / 2, y, z + bend), (x1, y, z - bend * 0.5)])
    return grain_strokes(name, strokes, GRAIN, radius=0.0011)


def _stack():
    """Les trois planches couchées les unes sur les autres, un peu de travers."""
    parts = []
    for level, (dx, dy, yaw, tint, seed) in enumerate(STACK):
        plank = turned(_plank(f"planche_{level}", tint, seed), (90, 0, yaw))  # dessus vers +z
        for part in plank:
            part.location.x += dx
            part.location.y += dy
            part.location.z += THICKNESS * (level + 0.5)
        parts += plank
    return parts


def _cords():
    """Deux tours de corde autour de la pile, aux coins arrondis."""
    top, w = THICKNESS * len(STACK) + 0.003, WIDTH / 2 + 0.006
    loop = [(-w, 0.004), (-w + 0.004, -0.002), (w - 0.004, -0.002), (w, 0.004), (w, top - 0.004),
            (w - 0.004, top), (-w + 0.004, top), (-w, top - 0.004), (-w, 0.004)]
    return [tube(f"corde_{i}", [(x + (z - top / 2) * 0.03, y, z) for y, z in loop],
                 [0.0048] * len(loop), flat_material("planche_corde", CORD), sides=5)
            for i, x in enumerate(CORD_X)]


def build():
    return _stack() + _cords()
