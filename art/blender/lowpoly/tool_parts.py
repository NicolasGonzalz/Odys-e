"""Pièces communes à tous les outils d'Odyssée : un même style pour la hache, la pioche, etc.

  TOOL_COLORS : la palette des outils (relevée sur les planches de concepts)
  haft        : manche en bois brut, légèrement courbé et ovale, tons en veines, fil, nœud, bout scié
  wood_details: fil, nœud et bout scié sur un manche (aussi pour les manches mesurés)
  grip        : bandage de poignée (rubans croisés + bord déchiré)
  lashing     : ligature qui fixe une tête sur le manche (rubans croisés + tours serrés)
  knapped     : pierre taillée à facettes (enveloppe convexe de points irréguliers)
  rust_spots  : plaques de rouille sur une face plate
"""
import math
import random

import bmesh
import bpy

from .materials import flat_material
from .organic import tube
from .shading import two_tone
from .shapes import bevel, cylinder, roughen
from .wood import end_grain, grain_lines, knot, streaked
from .wraps import crossed_wrap, frayed_edge

# Couleurs mesurées sur art/concepts/objets/outils-et-ressources.png (sRGB, 0 à 1).
TOOL_COLORS = {
    "bois": (0.49, 0.35, 0.25),
    "bois_sombre": (0.38, 0.26, 0.18),
    "tissu": (0.62, 0.55, 0.44),
    "corde": (0.50, 0.38, 0.25),
    # La pierre doit se distinguer du métal d'un coup d'œil : gris ardoise foncé et froid, quand le
    # métal est un gris clair et chaud (taché de rouille).
    "pierre": (0.40, 0.42, 0.45),
    "pierre_sombre": (0.27, 0.29, 0.32),
    "pierre_fil": (0.60, 0.62, 0.64),     # arête fraîchement taillée
    "tissu_sombre": (0.47, 0.41, 0.32),
    "metal": (0.68, 0.63, 0.57),
    "metal_sombre": (0.50, 0.48, 0.46),
    "metal_clair": (0.80, 0.77, 0.72),
    "rouille": (0.56, 0.32, 0.19),
    # Détails du bois (lowpoly/wood.py) : traits du fil, nœuds, bout scié.
    "bois_fil": (0.36, 0.24, 0.16),   # assez clair pour rester un trait, pas une fente
    "noeud": (0.30, 0.20, 0.13),
    "noeud_coeur": (0.19, 0.12, 0.08),
    "bois_bout": (0.68, 0.51, 0.35),
    "bois_cerne": (0.55, 0.40, 0.27),
    "bois_coeur": (0.45, 0.31, 0.20),
}


def material(name):
    return flat_material(f"outil_{name}", TOOL_COLORS[name])


# --- Manche -------------------------------------------------------------------------------------

# Bout scié, du cœur vers le bord : cœur sombre, bois clair, un cerne, bois clair.
SAWN_RINGS = [(0.16, TOOL_COLORS["bois_coeur"]), (0.5, TOOL_COLORS["bois_bout"]),
              (0.62, TOOL_COLORS["bois_cerne"]), (1.0, TOOL_COLORS["bois_bout"])]

HAFT_SIDES = 8        # section du manche : 8 pans
HAFT_FLATTEN = 0.86   # manche ovale, plus large dans le sens du coup (comme un vrai manche de hache)


def haft_points(length, bend=0.018, steps=7):
    """Ligne centrale d'un manche brut : légère courbe en arc (x) le long de z."""
    return [(bend * math.sin(math.pi * i / steps), 0.0, length * i / steps) for i in range(steps + 1)]


def haft(name, length, radius_bottom, radius_top, bend=0.018, seed=0, bare=(0.19, 0.5)):
    """Manche en bois brut : courbe légère, section ovale, rayon irrégulier, tons en veines,
    fil, nœud et bout scié. `bare` = (bas, haut) en fraction de la longueur : la partie de bois
    nu (non bandée) où poser les traits du fil et le nœud.
    Renvoie (pièces, center_x(z), radius_at(z)) pour y poser bandages et ligatures."""
    rng = random.Random(seed)
    points = haft_points(length, bend)
    radii = [radius_bottom + (radius_top - radius_bottom) * i / (len(points) - 1)
             + rng.uniform(-0.0015, 0.0015) for i in range(len(points))]
    wood = tube(f"{name}_bois", points, radii, material("bois"), sides=HAFT_SIDES,
                flatten=HAFT_FLATTEN)
    roughen(wood, 0.0006, seed)

    def center_x(z):
        return bend * math.sin(math.pi * max(0.0, min(1.0, z / length)))

    def radius_at(z):
        t = max(0.0, min(1.0, z / length))
        return radius_bottom + (radius_top - radius_bottom) * t

    details = wood_details(name, center_x, radius_at, length * bare[0], length * bare[1], seed=seed)
    return [shade_wood(wood, seed), *details], center_x, radius_at


def shade_wood(part, seed=0):
    """Tons du bois en veines, le long du manche (axe z)."""
    return streaked(part, TOOL_COLORS["bois"], TOOL_COLORS["bois_sombre"], "outil_bois", seed=seed)


def wood_details(name, center_x, radius_at, z_bottom, z_top, flatten=HAFT_FLATTEN, seed=0,
                 sawn_end=True):
    """Ce qui fait « bois » sur un manche : traits du fil et un nœud sur la partie nue,
    cernes sur le bout scié du pied (z = 0) si `sawn_end` (pas quand un embout le couvre)."""
    sides = HAFT_SIDES
    parts = grain_lines(name, center_x, radius_at, z_bottom, z_top, TOOL_COLORS["bois_fil"],
                        sides, flatten, count=8, seed=seed)
    knot_z = z_bottom + (z_top - z_bottom) * (0.35 + 0.3 * random.Random(seed).random())
    parts.append(knot(f"{name}_noeud", center_x, radius_at, knot_z, 4.4 + seed * 0.7, 0.006,
                      (TOOL_COLORS["noeud"], TOOL_COLORS["noeud_coeur"]), sides, flatten))
    if not sawn_end:
        return parts
    r = radius_at(0.0)
    parts += end_grain(f"{name}_bout", (center_x(0.0), 0.0, 0.0), (0, 0, -1), (0, 1, 0),
                       (r * flatten * 0.9, r * 0.9), SAWN_RINGS, sides, lift=0.0008, seed=seed)
    return parts


# --- Bandages et ligatures ----------------------------------------------------------------------

def grip(name, center_x, radius_at, z_bottom, z_top, seed=0, flatten=HAFT_FLATTEN):
    """Bandage de poignée : manchon, rubans croisés, bord inférieur déchiré (manche ovale)."""
    cloth = material("tissu")
    sleeve = tube(f"{name}_manchon", [(center_x(z), 0.0, z) for z in (z_bottom, z_top)],
                  [radius_at(z_bottom) + 0.002, radius_at(z_top) + 0.002], cloth, sides=7,
                  flatten=flatten)
    parts = [sleeve] + crossed_wrap(name, center_x, radius_at, z_bottom, z_top, cloth, turns=1.0,
                                    flatten=flatten)
    parts += frayed_edge(name, center_x, radius_at, z_bottom, cloth, seed=seed, flatten=flatten)
    return _cloth_tones(parts, seed)


def _cloth_tones(parts, seed):
    """Plis clairs et ombres du tissu : deux tons, comme le bois."""
    for index, part in enumerate(parts):
        two_tone(part, TOOL_COLORS["tissu"], TOOL_COLORS["tissu_sombre"], "outil_tissu",
                 threshold=0.3, spread=0.08, seed=seed + index)
    return parts


def cloth_band(name, center_x, radius_at, z_bottom, z_top, seed=0, flatten=HAFT_FLATTEN):
    """Bande de tissu enroulée sous une tête (comme sur l'image de la plaine)."""
    cloth = material("tissu")
    sleeve = tube(f"{name}_manchon", [(center_x(z), 0.0, z) for z in (z_bottom, z_top)],
                  [radius_at(z_bottom) + 0.003, radius_at(z_top) + 0.003], cloth, sides=7,
                  flatten=flatten)
    parts = ([sleeve] + crossed_wrap(name, center_x, radius_at, z_bottom, z_top, cloth, turns=0.8,
                                     flatten=flatten)
             + frayed_edge(name, center_x, radius_at, z_bottom, cloth, teeth=7, seed=seed,
                           flatten=flatten))
    return _cloth_tones(parts, seed)


def lashing(name, center_x, radius_at, z_bottom, z_top, extra=0.0):
    """Ligature en corde : tours serrés en haut et en bas, rubans croisés entre les deux.
    `extra` élargit la ligature quand elle passe par-dessus une tête plus épaisse que le manche."""
    cord = material("corde")

    def radius(z):
        return radius_at(z) + extra

    parts = crossed_wrap(f"{name}_croise", center_x, radius, z_bottom, z_top, cord, turns=0.75,
                         width=0.014)
    for index, z in enumerate((z_bottom + 0.008, z_top - 0.008)):
        ring = [(center_x(z), 0.0, z - 0.006), (center_x(z), 0.0, z + 0.006)]
        parts.append(tube(f"{name}_tour_{index}", ring, [radius(z) + 0.004] * 2, cord, sides=8))
    return parts


# --- Pierre taillée -----------------------------------------------------------------------------

def knapped(name, stations, seed=0, jitter=0.18):
    """Pierre taillée : à chaque station (x, demi-épaisseur y, demi-hauteur z, centre z), un anneau
    de points irréguliers ; l'enveloppe convexe donne les grandes facettes d'un éclat de silex."""
    rng = random.Random(seed)
    bm = bmesh.new()
    for x, half_y, half_z, center_z in stations:
        for k in range(6):
            angle = 2 * math.pi * k / 6 + rng.uniform(-0.3, 0.3)
            scale = 1 + rng.uniform(-jitter, jitter)
            bm.verts.new((x + rng.uniform(-0.004, 0.004), math.cos(angle) * half_y * scale,
                          center_z + math.sin(angle) * half_z * scale))
    bmesh.ops.convex_hull(bm, input=bm.verts[:])
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material("pierre"))
    two_tone(obj, TOOL_COLORS["pierre"], TOOL_COLORS["pierre_sombre"], "outil_pierre",
             dark_side=(0.6, 0.3, -0.7), threshold=0.1, spread=0.09, seed=seed)
    _sharp_edge(obj, stations)
    bevel(obj, 0.002)
    return obj


def _sharp_edge(obj, stations):
    """Le fil taillé de la pierre (sa station la plus fine) est plus clair, comme un éclat frais."""
    thin = min(stations, key=lambda s: s[1])[0]
    span = abs(stations[-1][0] - stations[0][0])
    obj.data.materials.append(flat_material("outil_pierre_fil", TOOL_COLORS["pierre_fil"]))
    edge = len(obj.data.materials) - 1
    for polygon in obj.data.polygons:
        if abs(polygon.center.x - thin) < span * 0.12:
            polygon.material_index = edge


# --- Rouille ------------------------------------------------------------------------------------

def rust_spots(name, spots, half_thickness_at, seed=0):
    """Plaques de rouille irrégulières, des deux côtés d'une pièce plate. `spots` : (x, z, rayon)."""
    rust = material("rouille")
    patches = []
    for index, (x, z, radius) in enumerate(spots):
        half = half_thickness_at(x) + 0.0008
        for side in (-1, 1):
            patch = cylinder(f"{name}_{index}_{side}", radius, 0.0015, rust, vertices=7,
                             location=(x, side * half, z), rotation_deg=(90, 0, index * 29 + seed))
            patches.append(roughen(patch, radius * 0.28, seed=seed + index * 2 + side))
    return patches
