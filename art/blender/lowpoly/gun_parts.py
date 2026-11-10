"""Pièces communes aux armes à feu d'Odyssée (pistolet, fusil à pompe), pointées vers +x.

Récupérées dans les ruines : métal gris foncé bronzé, usé et rouillé par endroits, bois des crosses
dans la palette commune des outils (lowpoly/tool_parts.py).

  GUN_COLORS    : métal bronzé (plus sombre et plus froid que le métal des outils)
  wood_piece    : pièce en bois vue de profil (crosse, poignée), tons en veines le long de x
  metal_piece   : pièce en métal vue de profil (carcasse, boîtier), deux tons et usure
  barrel        : canon (tube), avec sa bouche sombre
  trigger_group : pontet et détente
  sight         : guidon ou hausse
"""
from .materials import flat_material
from .organic import tube
from .profile import profile_solid
from .shading import blotches, two_tone
from .shapes import box, cylinder, wedge
from .tool_parts import TOOL_COLORS
from .wood import grain_strokes, streaked

GUN_COLORS = {
    "bronze": (0.30, 0.32, 0.35),
    "bronze_sombre": (0.19, 0.20, 0.23),
    "usure": (0.52, 0.53, 0.54),   # arêtes et zones de prise où le bronzage est parti
    "bouche": (0.06, 0.06, 0.07),
}


def gun_material(name):
    return flat_material(f"arme_{name}", GUN_COLORS[name])


def wood_piece(name, outline, thickness, grain=(), seed=0):
    """Pièce en bois d'après un profil (x, z), épaisseur selon y, arêtes arrondies (le bois se
    tient en main : rien de cubique). `grain` = traits du fil [(x0, x1, z)], sur les deux flancs :
    à garder à distance des bords (l'arrondi), sinon leur bout dépasse dans le vide."""
    wood = flat_material("outil_bois", TOOL_COLORS["bois"])
    piece = profile_solid(name, outline, lambda x: thickness, [wood], lambda x: 0,
                          cuts_x=_cuts(outline, 0.05), chamfer=thickness * 0.28, segments=2)
    streaked(piece, TOOL_COLORS["bois"], TOOL_COLORS["bois_sombre"], f"{name}_bois",
             axis=(1, 0, 0), dark_side=(0.2, 0.3, -1.0), threshold=0.2, streak=0.15, seed=seed)
    if not grain:
        return [piece]
    strokes = []
    for x0, x1, z in grain:
        for side in (-1, 1):
            y = side * (thickness / 2 + 0.0003)  # flancs plats, entre les arrondis
            strokes.append([(x0, y, z), ((x0 + x1) / 2, y, z + 0.003), (x1, y, z - 0.002)])
    return [piece, *grain_strokes(name, strokes, TOOL_COLORS["bois_fil"])]


def metal_piece(name, outline, thickness, wear=3, seed=0):
    """Pièce en métal bronzé d'après un profil (x, z) : deux tons, plaques d'usure et de rouille."""
    piece = profile_solid(name, outline, lambda x: thickness, [gun_material("bronze")], lambda x: 0,
                          cuts_x=_cuts(outline, 0.04), chamfer=0.004)
    return weathered(piece, wear, seed)


def weathered(part, wear=3, seed=0):
    """Deux tons du métal bronzé, puis usure claire et rouille en plaques."""
    two_tone(part, GUN_COLORS["bronze"], GUN_COLORS["bronze_sombre"], "arme_bronze",
             dark_side=(0.2, 0.3, -1.0), threshold=0.2, seed=seed)
    blotches(part, gun_material("usure"), count=wear, radius=0.012, seed=seed)
    return blotches(part, flat_material("outil_rouille", TOOL_COLORS["rouille"]), count=wear,
                    radius=0.016, seed=seed + 7)


def _cuts(outline, step):
    """Coupes verticales régulières : plus de faces, donc des veines et de l'usure plus fines."""
    xs = [x for x, _ in outline]
    start, end = min(xs), max(xs)
    count = int((end - start) / step)
    return [start + step * (i + 1) for i in range(count)]


def segmented(x0, x1, y, z, step=0.03):
    """Points d'un tube droit le long de x, tous les `step` m : assez de faces pour que l'usure
    et la rouille fassent des plaques, et pas des bandes sur toute la longueur."""
    count = max(1, round((x1 - x0) / step))
    return [(x0 + (x1 - x0) * i / count, y, z) for i in range(count + 1)]


def barrel(name, x0, x1, z, radius, y=0.0, sides=8, seed=0):
    """Canon le long de x, bronzé et usé, avec sa bouche noire au bout."""
    points = segmented(x0, x1, y, z)
    pipe = tube(name, points, [radius] * len(points), gun_material("bronze"), sides=sides)
    parts = [weathered(pipe, wear=1, seed=seed)]
    parts.append(cylinder(f"{name}_bouche", radius * 0.6, 0.002, gun_material("bouche"),
                          vertices=sides, location=(x1 + 0.0006, y, z), rotation_deg=(0, 90, 0)))
    return parts


def trigger_group(name, x, z, length=0.055, depth=0.03):
    """Pontet (boucle sous la carcasse) et détente, à partir de l'avant du pontet (x, z)."""
    dark = gun_material("bronze_sombre")
    guard = [(x, 0, z), (x - 0.004, 0, z - depth * 0.9), (x - length * 0.5, 0, z - depth),
             (x - length, 0, z - depth * 0.7), (x - length - 0.004, 0, z)]
    trigger = wedge(f"{name}_detente", 0.026, (0.006, 0.008), (0.005, 0.003), dark,
                    location=(x - length * 0.45, 0, z - 0.002), rotation_deg=(0, 75, 0))
    return [tube(f"{name}_pontet", guard, [0.003] * len(guard), dark, sides=5), trigger]


def sight(name, x, z, size=(0.008, 0.006, 0.01)):
    """Guidon ou hausse : un petit bloc sur le dessus."""
    return box(name, size, gun_material("bronze_sombre"), location=(x, 0, z + size[2] / 2))
