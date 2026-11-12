"""Coffre en métal : une caisse militaire d'avant, retrouvée dans les ruines, que la végétation
commence à reprendre. Plus solide que le coffre en bois (plus tard : verrou, butin rare).

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/coffre_metal.png).
Caisse grise aux arêtes chanfreinées, couvercle à rebord, deux sangles de renfort, deux fermoirs
et un moraillon au centre, poignées encastrées sur les côtés, plaque sur le devant, coins
renforcés ; rouille en grandes plaques, lianes qui grimpent sur un angle et un côté.
0,9 × 0,55 m, 0,62 m de haut. Pivot au pied.
"""
import bmesh
import bpy

from lowpoly.carpentry import stain
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import bevel, box
from lowpoly.tool_parts import TOOL_COLORS, material
from lowpoly.vines import vine

ICON = True
VIEW = (-0.6, -1.0, 0.55)

LENGTH, DEPTH = 0.9, 0.55
BODY_HEIGHT = 0.44
LID_HEIGHT = 0.18
GREY = (0.76, 0.75, 0.72)       # gris clair du concept
GREY_DARK = (0.56, 0.56, 0.55)
STRAP = (0.58, 0.58, 0.58)
RUST = (0.66, 0.36, 0.20)


def _shell(name, size, center, seed, chamfer=0.025):
    """Boîte subdivisée (la rouille fait des plaques, pas des faces entières), arêtes chanfreinées."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=3, use_grid_fill=True)
    for v in bm.verts:
        v.co = (v.co.x * size[0], v.co.y * size[1], v.co.z * size[2])
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    shell = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(shell)
    shell.location = center
    shell.data.materials.append(flat_material("coffre_metal_gris", GREY))
    bevel(shell, chamfer, segments=1 if chamfer > 0.03 else 2)
    two_tone(shell, GREY, GREY_DARK, "coffre_metal_gris", dark_side=(0.4, 0.6, -0.4),
             threshold=0.1, seed=seed)
    return blotches(shell, flat_material("coffre_metal_rouille", RUST), count=3, radius=0.06,
                    seed=seed)


def _hardware():
    """Sangles, fermoirs, moraillon, poignées, plaque avant, coins renforcés."""
    dark = flat_material("coffre_metal_fer", STRAP)
    top = BODY_HEIGHT + LID_HEIGHT
    parts = []
    for i, x in enumerate((-0.26, 0.26)):
        for side in (-1, 1):  # sangle verticale, devant et derrière
            parts.append(box(f"sangle_{i}_{side}", (0.07, 0.012, top), dark,
                             location=(x, side * (DEPTH / 2 + 0.006), top / 2)))
        parts.append(box(f"sangle_dessus_{i}", (0.07, DEPTH + 0.024, 0.012), dark,
                         location=(x, 0, top + 0.006)))
        parts.append(bevel(box(f"fermoir_{i}", (0.06, 0.03, 0.09), material("metal_sombre"),
                               location=(x, -(DEPTH / 2 + 0.02), BODY_HEIGHT)), 0.006))
    parts.append(bevel(box("moraillon", (0.08, 0.035, 0.1), material("metal"),
                           location=(0, -(DEPTH / 2 + 0.02), BODY_HEIGHT - 0.01)), 0.008))
    parts.append(box("plaque", (0.3, 0.008, 0.14), dark,
                     location=(0, -(DEPTH / 2 + 0.004), 0.17)))
    for side in (-1, 1):
        parts.append(box(f"poignee_{side}", (0.012, 0.2, 0.08), dark,
                         location=(side * (LENGTH / 2 + 0.006), 0, BODY_HEIGHT * 0.62)))
    return parts + _corners(dark)


def _corners(dark):
    parts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            parts.append(bevel(box(f"coin_{sx}_{sy}", (0.06, 0.06, 0.06), dark,
                                   location=(sx * (LENGTH / 2 - 0.02), sy * (DEPTH / 2 - 0.02), 0.03)),
                               0.008))
    return parts


def _rust_patches():
    """Plaques de rouille irrégulières sur le couvercle et la face avant, comme sur le concept."""
    top = BODY_HEIGHT + LID_HEIGHT + 0.0055
    parts = [stain(f"rouille_dessus_{i}", (x, y, top), r, RUST, seed=20 + i)
             for i, (x, y, r) in enumerate(((-0.12, -0.08, 0.07), (0.1, 0.1, 0.055),
                                            (0.36, -0.12, 0.045), (-0.36, 0.12, 0.05)))]
    for i, (x, z, r) in enumerate(((-0.38, 0.32, 0.05), (0.12, 0.1, 0.06), (0.38, 0.2, 0.04),
                                   (-0.12, 0.38, 0.035))):
        patch = stain(f"rouille_face_{i}", (0.0, 0.0, 0.0), r, RUST, seed=30 + i)
        parts += posed([patch], (90, 0, 0), (x, -(DEPTH / 2 + 0.0035), z))
    return parts


def _rust_streaks():
    """Coulures de rouille sous les fermoirs, sur la face avant."""
    parts = []
    for i, x in enumerate((-0.26, 0.0, 0.26)):
        streak = stain(f"coulure_{i}", (0.0, 0.0, 0.0), 0.04, RUST, seed=10 + i)
        streak.scale = (0.6, 1.8, 1.0)
        parts += posed([streak], (90, 0, 0), (x, -(DEPTH / 2 + 0.014), BODY_HEIGHT - 0.12))
    return parts


def _vines():
    """Lianes : l'une grimpe sur l'angle avant droit et passe sur le couvercle, l'autre sur le
    côté gauche."""
    top = BODY_HEIGHT + LID_HEIGHT
    corner = [(LENGTH / 2 + 0.012, -DEPTH / 2 + 0.03, 0.0), (LENGTH / 2 + 0.015, -DEPTH / 2 + 0.05, 0.2),
              (LENGTH / 2 + 0.012, -DEPTH / 2 + 0.02, 0.4), (LENGTH / 2 + 0.01, -DEPTH / 2 + 0.06, top),
              (LENGTH / 2 - 0.12, -DEPTH / 2 + 0.1, top + 0.012)]
    left = [(-LENGTH / 2 - 0.012, 0.15, 0.0), (-LENGTH / 2 - 0.014, 0.05, 0.15),
            (-LENGTH / 2 - 0.012, 0.1, 0.3)]
    return (vine("liane_angle", corner, (1, -0.3, 0), leaves=22, size=0.055, seed=3)
            + vine("liane_cote", left, (-1, 0, 0), leaves=10, size=0.045, seed=5))


def build():
    body = _shell("coffre_corps", (LENGTH, DEPTH, BODY_HEIGHT), (0, 0, BODY_HEIGHT / 2), seed=1)
    lid = _shell("coffre_couvercle", (LENGTH + 0.02, DEPTH + 0.02, LID_HEIGHT),
                 (0, 0, BODY_HEIGHT + LID_HEIGHT / 2 + 0.004), seed=2, chamfer=0.06)
    return [body, lid] + _hardware() + _rust_patches() + _rust_streaks() + _vines()
