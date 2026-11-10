"""Carte (objet) : on la reçoit au réveil, presque vierge. Seule la zone de départ (L'Abri) y est
dessinée ; le reste se dévoile en explorant, à reporter sur une table de cartographie
(docs/GAMEPLAY.md § 3). Ouverte avec la touche M ou depuis l'inventaire.

Pas de concept dédié : papier vieilli, plié en accordéon sur trois volets, taches et coin déchiré.
Dessin à l'encre sur le premier volet : la tache verte de l'Abri, la rivière, un chemin en
pointillés, une croix rouge sur le centre de stase. Rose des vents dans un coin. 0,30 × 0,22 m.
"""
import math
import random

import bpy

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.outline import without_outline
from lowpoly.shading import two_tone

ICON = True
VIEW = (0.15, -0.8, 1.0)   # de haut : on doit voir le dessin
OUTLINE_THICKNESS = 0.002

WIDTH, DEPTH = 0.30, 0.22  # carte dépliée (m)
PANELS = 3                 # volets de l'accordéon (le long de x)
FOLD = 0.006               # hauteur des plis
THICKNESS = 0.0012
LIFT = 0.0006              # l'encre est posée juste au-dessus du papier

PAPER = (0.92, 0.87, 0.74)
PAPER_DARK = (0.82, 0.76, 0.62)
STAIN = (0.80, 0.70, 0.52)
INK_GREEN = (0.55, 0.66, 0.40)
INK_GREEN_DARK = (0.27, 0.38, 0.20)
INK_BLUE = (0.30, 0.45, 0.60)
INK_BROWN = (0.36, 0.26, 0.18)
INK_RED = (0.70, 0.18, 0.14)

# Dessin de la zone de départ, sur le premier volet (x de 0 à 0,1), en mètres.
ZONE = [(0.020, -0.050), (0.034, -0.066), (0.055, -0.072), (0.072, -0.068), (0.088, -0.055),
        (0.086, -0.034), (0.092, -0.010), (0.088, 0.012), (0.080, 0.030), (0.066, 0.036),
        (0.050, 0.052), (0.034, 0.046), (0.022, 0.035), (0.018, 0.014), (0.012, -0.008),
        (0.016, -0.030)]
RIVER = [(0.004, 0.065), (0.030, 0.045), (0.050, 0.060), (0.075, 0.040), (0.097, 0.050)]
PATH = [(0.030, -0.035), (0.045, -0.015), (0.058, 0.005), (0.070, 0.022), (0.085, 0.032)]
STASIS = (0.040, -0.030)   # le centre de stase, point de départ
COMPASS = (0.262, 0.070)   # rose des vents, coin du dernier volet
STAINS = [(0.165, -0.035, 0.022), (0.245, -0.075, 0.014)]  # (x, y, rayon), loin des plis


def fold_height(x):
    """Hauteur du papier : pli en accordéon (un pic à chaque bord de volet)."""
    panel = WIDTH / PANELS
    t = (x % panel) / panel
    up = (int(x // panel) % 2) == 0
    return FOLD * (t if up else 1 - t)


def _paper_mesh():
    """Grille de papier pliée en accordéon, avec un coin déchiré (en haut à droite)."""
    columns, rows = PANELS * 4, 6
    vertices = []
    for j in range(rows + 1):
        for i in range(columns + 1):
            x, y = WIDTH * i / columns, -DEPTH / 2 + DEPTH * j / rows
            vertices.append((x, y, fold_height(x)))
    faces = [(j * (columns + 1) + i, j * (columns + 1) + i + 1, (j + 1) * (columns + 1) + i + 1,
              (j + 1) * (columns + 1) + i) for j in range(rows) for i in range(columns)]
    corner = faces.pop()             # coin déchiré : la dernière case devient un triangle
    faces.append((corner[0], corner[1], corner[3]))
    mesh = bpy.data.meshes.new("carte")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    return mesh


def _paper():
    """Le papier : épaissi vers le bas, deux tons selon les plis, quelques taches."""
    paper = bpy.data.objects.new("carte", _paper_mesh())
    bpy.context.scene.collection.objects.link(paper)
    paper.data.materials.append(flat_material("carte_papier", PAPER))
    solid = paper.modifiers.new("Epaisseur", "SOLIDIFY")
    solid.thickness = THICKNESS
    solid.offset = -1.0
    return two_tone(paper, PAPER, PAPER_DARK, "carte_papier", dark_side=(1.0, 0.0, 0.25),
                    threshold=0.04, spread=0.02, seed=2)


def _stains():
    """Taches rondes et irrégulières (eau, café, doigts sales), posées comme l'encre."""
    rng = random.Random(7)
    stains = []
    for index, (cx, cy, radius) in enumerate(STAINS):
        outline = [(cx + math.cos(a) * radius * rng.uniform(0.75, 1.15),
                    cy + math.sin(a) * radius * rng.uniform(0.75, 1.15))
                   for a in (math.tau * k / 9 for k in range(9))]
        stains.append(_ink_area(f"tache_{index}", outline, STAIN))
    return stains


def _on_paper(points):
    """Points du dessin posés sur le papier plié."""
    return [(x, y, fold_height(x) + LIFT) for x, y in points]


def _ink_area(name, outline, color):
    """Zone coloriée (éventail de triangles depuis son centre)."""
    cx = sum(x for x, _ in outline) / len(outline)
    cy = sum(y for _, y in outline) / len(outline)
    vertices = _on_paper([(cx, cy)] + outline)
    faces = [(0, i + 1, (i + 1) % len(outline) + 1) for i in range(len(outline))]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(flat_material(f"carte_{name}", color))
    return without_outline(obj)


def _ink_line(name, points, color, radius=0.0009):
    line = tube(name, _on_paper(points), [radius] * len(points), flat_material(f"carte_{name}", color),
                sides=3)
    return without_outline(line)


def _dashed(name, points, color):
    """Chemin en pointillés : un trait sur deux entre les points."""
    return [_ink_line(f"{name}_{i}", [a, b], color, radius=0.0011)
            for i, (a, b) in enumerate(zip(points, points[1:])) if i % 2 == 0]


def _cross(center, size=0.007):
    x, y = center
    return [_ink_line(f"croix_{i}", [(x - dx, y - dy), (x + dx, y + dy)], INK_RED, radius=0.0011)
            for i, (dx, dy) in enumerate(((size, size), (size, -size)))]


def _compass(center, size=0.016):
    """Rose des vents : quatre branches, le nord (vers +y) plus long."""
    x, y = center
    rays = [(0, 1.0), (1.0, 0), (0, -0.7), (-0.7, 0)]
    lines = [_ink_line(f"rose_{i}", [(x, y), (x + dx * size, y + dy * size)], INK_BROWN)
             for i, (dx, dy) in enumerate(rays)]
    ring = [(x + math.cos(a) * size * 0.45, y + math.sin(a) * size * 0.45)
            for a in (math.tau * k / 8 for k in range(9))]
    return lines + [_ink_line("rose_cercle", ring, INK_BROWN, radius=0.0006)]


def _drawing():
    """Tout ce que la carte montre au réveil : la zone de départ, et rien d'autre."""
    rng = random.Random(1)
    zone = [(x + rng.uniform(-0.002, 0.002), y + rng.uniform(-0.002, 0.002)) for x, y in ZONE]
    edge = _ink_line("zone_contour", zone + zone[:1], INK_GREEN_DARK, radius=0.0009)
    return ([_ink_area("zone_depart", zone, INK_GREEN), edge, _ink_line("riviere", RIVER, INK_BLUE, 0.0014)]
            + _dashed("chemin", PATH, INK_BROWN) + _cross(STASIS) + _compass(COMPASS))


def build():
    return [_paper()] + _stains() + _drawing()
