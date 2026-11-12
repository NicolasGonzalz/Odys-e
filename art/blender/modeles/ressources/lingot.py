"""Lingot de métal (ressource transformée) : le minerai fondu au foyer et coulé dans un moule.
C'est lui qui sert aux outils de métal (palier II du site : « Le métal »).

Pas de concept dédié. Fait main, donc imparfait : forme de moule ouvert (plus large en haut qu'en
bas), dessus un peu creusé par le retrait du métal en refroidissant, surface martelée plus claire,
flancs gris sombre, bas noirci par la suie du foyer. Pas de rouille : il sort de la forge.
Longueur 0,15 m.
"""
import random

import bmesh
import bpy

from lowpoly.materials import flat_material
from lowpoly.shading import two_tone
from lowpoly.carpentry import stain
from lowpoly.outline import without_outline
from lowpoly.shapes import bevel, box

ICON = True
VIEW = (0.5, -1.0, 0.8)

BOTTOM = (0.115, 0.042)  # longueur, largeur du fond du moule (m) : bords bien évasés
TOP = (0.165, 0.078)     # longueur, largeur du dessus
HEIGHT = 0.042
SHRINK = 0.008           # creux du dessus, au centre (retrait du métal en refroidissant)

METAL = (0.67, 0.64, 0.60)      # fer chaud, pas un gris froid
METAL_DARK = (0.46, 0.44, 0.42)
HAMMERED = (0.82, 0.79, 0.74)   # marques de marteau sur le dessus
SOOT = (0.24, 0.22, 0.21)
STAMP = (0.30, 0.28, 0.27)


def _block():
    """Bloc subdivisé (assez de sommets pour creuser le dessus et bosseler les flancs)."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=2, use_grid_fill=True)
    return bm


def _shape(bm, rng):
    """Taille de moule (évasée vers le haut), dessus creusé au centre, petites bosses partout."""
    for vertex in bm.verts:
        height = vertex.co.z + 0.5   # 0 au fond, 1 en haut
        length = BOTTOM[0] + (TOP[0] - BOTTOM[0]) * height
        width = BOTTOM[1] + (TOP[1] - BOTTOM[1]) * height
        x, y = vertex.co.x * length, vertex.co.y * width
        z = height * HEIGHT
        if height > 0.99:  # dessus : plus creusé au centre qu'aux bords
            z -= SHRINK * (1 - abs(vertex.co.x * 2) ** 2) * (1 - abs(vertex.co.y * 2) ** 2)
        vertex.co = (x + rng.uniform(-0.0012, 0.0012), y + rng.uniform(-0.0012, 0.0012),
                     z + rng.uniform(-0.0008, 0.0008))


def _mesh(bm, name):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(flat_material("lingot_metal", METAL))
    return obj


def _colors(ingot):
    """Flancs sombres, dessus martelé clair, suie vers le bas."""
    two_tone(ingot, METAL, METAL_DARK, "lingot_metal", dark_side=(0.4, 0.6, -0.6), threshold=0.1,
             spread=0.1, seed=4)
    ingot.data.materials.append(flat_material("lingot_martele", HAMMERED))
    hammered = len(ingot.data.materials) - 1
    rng = random.Random(5)
    for polygon in ingot.data.polygons:
        if polygon.normal.z > 0.8 and rng.random() < 0.35:
            polygon.material_index = hammered
    ingot.data.materials.append(flat_material("lingot_suie", SOOT))
    soot = len(ingot.data.materials) - 1
    for polygon in ingot.data.polygons:
        if polygon.center.z < HEIGHT * 0.25 and rng.random() < 0.6:
            polygon.material_index = soot
    return ingot


def build():
    bm = _block()
    _shape(bm, random.Random(3))
    ingot = _mesh(bm, "lingot")
    bevel(ingot, 0.005, segments=2)  # arêtes adoucies : coulé, pas usiné
    return [_colors(ingot)] + _marks()


def _marks():
    """Poinçon du forgeron (un petit carré creusé) et deux coups de marteau sur le dessus."""
    z = HEIGHT - SHRINK * 0.4
    stamp = flat_material("lingot_poincon", STAMP)
    parts = [box("poincon", (0.016, 0.016, 0.003), stamp, location=(-0.045, 0.012, z),
                 rotation_deg=(0, 0, 12))]
    for i, (x, y) in enumerate(((0.03, -0.012), (0.05, 0.016))):
        parts.append(without_outline(stain(f"coup_{i}", (x, y, z + 0.0012), 0.009, HAMMERED,
                                           seed=i)))
    return parts
