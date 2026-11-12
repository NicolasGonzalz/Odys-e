"""Mobilier en bois de récupération : planches, poutres, taches, tôles. Commun aux postes (établi,
forge, table de cartographie) et à tout ce qui se construit plus tard.

Le style des planches de concepts : grosses planches usées, bois plus sombre que les manches
(il a pris la pluie), traits du fil, bouts clairs, taches de rouille là où traînait le métal.

  WOOD_COLORS : bois de mobilier, ton clair et ton sombre
  board       : une planche ou une poutre, le long de x, centrée à l'origine, dessus vers +z
  stain       : tache plate et irrégulière posée sur une surface horizontale
  sheet       : tôle plate rouillée, d'après un contour (vue de dessus)
"""
import math
import random

import bpy

from .materials import flat_material
from .outline import without_outline
from .profile import profile_solid
from .shading import two_tone
from .shapes import roughen
from .tool_parts import TOOL_COLORS, material
from .wood import grain_strokes, streaked

WOOD_COLORS = {
    "bois": (0.64, 0.45, 0.29),        # mobilier : brun orangé des planches de concepts
    "bois_sombre": (0.47, 0.32, 0.21),
    "poutre": (0.45, 0.32, 0.22),      # pieds et traverses : bois plus sombre, vieilli
    "poutre_sombre": (0.32, 0.23, 0.16),
    "bout": (0.66, 0.50, 0.34),
    "fil": (0.30, 0.20, 0.13),
}


def board(name, length, width, thickness, colors=("bois", "bois_sombre"), tint=1.0, seed=0,
          grain=3):
    """Planche (ou poutre si large et épaisse) le long de x, centrée à l'origine, dessus vers +z.
    Tons en veines le long du fil, bouts sciés plus clairs, `grain` traits du fil sur le dessus."""
    rng = random.Random(seed)
    skew = rng.uniform(-0.006, 0.006)  # bouts sciés à la main : pas tout à fait d'équerre
    outline = [(-length / 2 + skew, -width / 2), (length / 2 + skew, -width / 2),
               (length / 2 - skew, width / 2), (-length / 2 - skew, width / 2)]
    steps = max(1, int(length / 0.12))  # des tronçons : assez de faces pour les veines du bois
    cuts = [-length / 2 + length * i / steps for i in range(1, steps)]
    piece = profile_solid(name, outline, lambda _: thickness, [material("bois")], lambda _: 0,
                          cuts_x=cuts, chamfer=min(0.006, thickness * 0.2))
    roughen(piece, 0.0012, seed)
    light, dark = (tuple(min(1.0, c * tint) for c in WOOD_COLORS[k]) for k in colors)
    streaked(piece, light, dark, f"{name}_bois", axis=(1, 0, 0), dark_side=(0.3, -0.6, -1.0),
             threshold=0.2, streak=0.18, spread=0.12, seed=seed)
    _light_ends(piece)
    parts = [piece] + _grain(name, length, width, thickness, grain, rng)
    for part in parts:  # le profil est à plat dans (x, y) avant d'être couché : dessus vers +z
        part.rotation_euler = (math.radians(90), 0, 0)
    return parts


def _light_ends(piece):
    piece.data.materials.append(flat_material("mobilier_bout", WOOD_COLORS["bout"]))
    end = len(piece.data.materials) - 1
    for polygon in piece.data.polygons:
        if abs(polygon.normal.x) > 0.8:
            polygon.material_index = end


def _grain(name, length, width, thickness, count, rng):
    """Traits du fil sur la face du dessus (y = +épaisseur / 2 avant d'être couchée)."""
    y = thickness / 2 + 0.0004
    strokes = []
    for _ in range(count):
        z = rng.uniform(-width * 0.35, width * 0.35)
        x0, x1 = -length / 2 + rng.uniform(0.04, 0.2), length / 2 - rng.uniform(0.04, 0.2)
        bend = rng.uniform(-0.008, 0.008)
        strokes.append([(x0, y, z), ((x0 + x1) / 2, y, z + bend), (x1, y, z - bend * 0.5)])
    return grain_strokes(name, strokes, WOOD_COLORS["fil"], radius=0.0016) if strokes else []


def stain(name, center, radius, color=None, seed=0):
    """Tache plate et irrégulière (rouille, brûlure, huile) posée à plat en `center`."""
    rng = random.Random(seed)
    cx, cy, cz = center
    ring = [(cx + math.cos(a) * radius * rng.uniform(0.6, 1.2),
             cy + math.sin(a) * radius * rng.uniform(0.6, 1.2), cz)
            for a in (math.tau * k / 9 for k in range(9))]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([center] + ring, [], [(0, i + 1, (i + 1) % 9 + 1) for i in range(9)])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(flat_material(f"{name}_couleur", color or TOOL_COLORS["rouille"]))
    return without_outline(obj)


def sheet(name, outline, thickness=0.004, rust=4, seed=0):
    """Tôle de récupération d'après un contour vu de dessus (x, y), posée à plat (dessus vers +z),
    grise et tachée de rouille en plaques rondes. Renvoie la tôle et ses taches."""
    flat = [(x, -y) for x, y in outline]  # couchée par la rotation : y redevient y
    plate = profile_solid(name, flat, lambda _: thickness, [material("metal")], lambda _: 0,
                          cuts_x=_cuts(flat, 0.06), chamfer=0.0015)
    two_tone(plate, TOOL_COLORS["metal"], TOOL_COLORS["metal_sombre"], f"{name}_metal",
             dark_side=(0.2, 0.4, -1.0), threshold=0.2, seed=seed)
    plate.rotation_euler = (math.radians(90), 0, 0)
    return [plate] + _rust_on(name, outline, thickness, rust, seed)


def _rust_on(name, outline, thickness, count, seed):
    """Taches de rouille rondes, posées sur le dessus de la tôle (à l'intérieur du contour)."""
    rng = random.Random(seed)
    xs, ys = [x for x, _ in outline], [y for _, y in outline]
    spots = []
    for i in range(count):
        x = rng.uniform(min(xs) * 0.6, max(xs) * 0.6)
        y = rng.uniform(min(ys) * 0.6, max(ys) * 0.6)
        radius = rng.uniform(0.02, 0.05)
        spots.append(stain(f"{name}_rouille_{i}", (x, y, thickness / 2 + 0.0006), radius,
                           seed=seed + i))
    return spots


def _cuts(outline, step):
    xs = [x for x, _ in outline]
    return [min(xs) + step * (i + 1) for i in range(int((max(xs) - min(xs)) / step))]
