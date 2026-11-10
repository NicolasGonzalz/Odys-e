"""Lance de Gaïa : une hampe de survivant, à la pointe en cristal de la forêt du Cœur.

Références : art/concepts/ambiances/zones-1-a-5-planche-a.jpeg et planche-b (Le Cœur) : longue
hampe en bois, pointe taillée en cristal violet, ligaturée, prise dans des feuilles et des éclats
lumineux de Gaïa. Les Villages sur pilotis (zones-1-3-5-lieux.jpeg) montrent la version simple,
pointe en métal : même modèle, en changeant TIP_COLORS.

Debout selon z, pointe en haut. Hauteur 1,6 m. Les feuilles et éclats sont émissifs.
"""
import math

from mathutils import Vector

from lowpoly.finish import turned
from lowpoly.materials import flat_material
from lowpoly.shading import two_tone
from lowpoly.shapes import cone
from lowpoly.tool_parts import cloth_band, grip, haft, knapped, lashing

ICON = True
VIEW = (0.4, -1.0, 0.15)

LENGTH = 1.4
TIP_LENGTH = 0.2
CRYSTAL = (0.64, 0.56, 0.80)
CRYSTAL_DARK = (0.38, 0.31, 0.52)
LEAF = (0.50, 0.95, 0.42)
SPARK = (0.85, 0.45, 1.0)

# Stations de la pointe (x, demi-épaisseur, demi-largeur, centre) : construite couchée selon x.
TIP = [(0.0, 0.012, 0.014, 0.0), (0.05, 0.011, 0.03, 0.0), (0.12, 0.007, 0.022, 0.0),
       (0.2, 0.001, 0.002, 0.0)]


def _tip():
    """Pointe en cristal taillé, à facettes, deux tons violets et un peu lumineuse."""
    tip = knapped("pointe", TIP, seed=9, jitter=0.12)
    two_tone(tip, CRYSTAL, CRYSTAL_DARK, "lance_cristal", dark_side=(0.2, 0.6, -0.8),
             threshold=0.0, spread=0.12, seed=9)
    tip.data.materials.append(flat_material("lance_cristal_eclat", CRYSTAL, emission=1.5))
    for polygon in tip.data.polygons:
        if polygon.center.x > 0.12:
            polygon.material_index = len(tip.data.materials) - 1
    turned([tip], (0, -90, 0))  # la pointe vers le haut
    tip.location.z += LENGTH - 0.03
    return tip


# Pousses de Gaïa : (hauteur sous la pointe, angle autour de la hampe, feuille ou éclat).
GROWTH = [(0.03, 0.0, "feuille"), (0.05, 2.3, "feuille"), (0.07, 4.4, "eclat"),
          (0.09, 1.2, "feuille"), (0.11, 3.4, "feuille"), (0.13, 5.5, "eclat"),
          (0.15, 0.6, "feuille"), (0.18, 2.9, "feuille"), (0.2, 4.9, "eclat")]


def _sprout(name, center_x, radius_at, depth, angle, kind):
    """Une feuille (longue, plate, verte) ou un éclat (court, violet), tournés vers l'extérieur
    et vers le haut, tous lumineux."""
    z = LENGTH - depth
    side = Vector((math.cos(angle), math.sin(angle), 0.0))
    out = (side + Vector((0, 0, 0.8))).normalized()
    leaf = kind == "feuille"
    material = flat_material(f"lance_{kind}", LEAF if leaf else SPARK, emission=2.5)
    piece = cone(name, 0.019 if leaf else 0.008, 0.075 if leaf else 0.03, material, vertices=4)
    piece.scale = (1.0, 0.22, 1.0) if leaf else (1.0, 1.0, 1.0)
    piece.location = Vector((center_x(z), 0.0, z)) + side * radius_at(z) + out * 0.03
    piece.rotation_euler = out.to_track_quat("Z", "Y").to_euler()
    return piece


def _gaia_growth(center_x, radius_at):
    """Feuilles lumineuses et éclats violets qui poussent autour de la ligature."""
    return [_sprout(f"pousse_{i}", center_x, radius_at, depth, angle, kind)
            for i, (depth, angle, kind) in enumerate(GROWTH)]


def build():
    parts, center_x, radius_at = haft("hampe", LENGTH, 0.019, 0.016, bend=0.012, seed=7,
                                      bare=(0.4, 0.82))
    parts.append(_tip())
    parts += lashing("ligature", center_x, radius_at, LENGTH - 0.08, LENGTH - 0.005, extra=0.004)
    parts += cloth_band("bande", center_x, radius_at, LENGTH - 0.2, LENGTH - 0.1, seed=3)
    parts += grip("poignee", center_x, radius_at, 0.35, 0.55, seed=4)
    return parts + _gaia_growth(center_x, radius_at)
