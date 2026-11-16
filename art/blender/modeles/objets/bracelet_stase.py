"""Bracelet de stase (docs/BRACELET.md) : le bracelet de capsule d'Odyssée, soudé au poignet du
survivant. Il porte l'inventaire, la fabrication, le carnet de survie, l'histoire et la carte.

Sans concept : brassard épais en métal gris-bleu d'Odyssée (un peu ovale, comme un poignet), boîtier
à écran sur le dessus (lueur cyan), petite lentille de projecteur à l'avant (option hologramme),
cellules solaires sur les flancs (recharge solaire), port de données sur le côté (on y branche les
clés USB), soudure grossière dessous (il ne s'enlève pas), usure claire sur les arêtes.
Axe du bras : x (la main vers +x). ~9 cm de diamètre, 4,5 cm de large. Choix de design à valider.
"""
import math

import bmesh
import bpy

from lowpoly.materials import flat_material
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import bevel, blob, box, cylinder

ICON = True
VIEW = (0.55, -1.0, 0.65)
OUTLINE_THICKNESS = 0.0012  # petit objet : trait fin

INNER = (0.032, 0.037)      # rayons intérieurs (vertical, horizontal) : poignet ovale
THICKNESS = 0.0075
HALF_WIDTH = 0.0225
SEGMENTS = 16

BAND = (0.36, 0.41, 0.48)       # métal gris-bleu d'Odyssée
BAND_DARK = (0.22, 0.25, 0.30)
LINER = (0.10, 0.10, 0.11)      # garniture intérieure
CASE = (0.24, 0.26, 0.29)
CASE_DARK = (0.15, 0.16, 0.18)
WEAR = (0.62, 0.65, 0.68)       # métal mis à nu sur les arêtes
SCREEN = (0.06, 0.22, 0.26)
GLOW = (0.35, 0.92, 1.0)
SOLAR = (0.07, 0.11, 0.28)
SOLAR_LINES = (0.34, 0.44, 0.70)
PORT_RING = (0.95, 0.55, 0.12)  # repère orange du port de données
WELD = (0.30, 0.27, 0.25)

TOP = INNER[0] + THICKNESS      # dessus du brassard


def _ring_point(angle, grow, x):
    """Point du brassard à l'angle `angle` (0 en haut), décalé de `grow` vers l'extérieur."""
    return (x, (INNER[1] + grow) * math.sin(angle), (INNER[0] + grow) * math.cos(angle))


def _cuff():
    """Le brassard : un anneau à section rectangulaire, ovale, facettes franches."""
    bm = bmesh.new()
    rings = []
    for i in range(SEGMENTS):
        angle = math.tau * i / SEGMENTS
        rings.append([bm.verts.new(_ring_point(angle, grow, x))
                      for grow, x in ((0, -HALF_WIDTH), (THICKNESS, -HALF_WIDTH),
                                      (THICKNESS, HALF_WIDTH), (0, HALF_WIDTH))])
    for i in range(SEGMENTS):
        a, b = rings[i], rings[(i + 1) % SEGMENTS]
        for k in range(4):
            bm.faces.new((a[k], b[k], b[(k + 1) % 4], a[(k + 1) % 4]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)  # avant de peindre selon les normales
    mesh = bpy.data.meshes.new("brassard")
    bm.to_mesh(mesh)
    bm.free()
    cuff = bpy.data.objects.new("brassard", mesh)
    bpy.context.scene.collection.objects.link(cuff)
    cuff.data.materials.append(flat_material("bracelet_metal", BAND))
    two_tone(cuff, BAND, BAND_DARK, "bracelet_metal", dark_side=(0.2, 0.5, -0.8), threshold=0.0,
             seed=3)
    _paint_liner(cuff)
    blotches(cuff, flat_material("bracelet_usure", WEAR), count=4, radius=0.008, seed=9)
    return cuff


def _paint_liner(cuff):
    """Faces intérieures (contre le poignet) : la garniture sombre. Repérées par leur place sur
    l'ellipse du brassard, pas par leur normale."""
    liner = len(cuff.data.materials)
    cuff.data.materials.append(flat_material("bracelet_garniture", LINER))
    for polygon in cuff.data.polygons:
        c = polygon.center
        on_ellipse = math.hypot(c.y / INNER[1], c.z / INNER[0])
        if on_ellipse < 1.0 + THICKNESS / (4 * INNER[0]) and abs(polygon.normal.x) < 0.5:
            polygon.material_index = liner


def _case():
    """Boîtier du dessus, son écran (lueur cyan et trois lignes d'interface) et sa lentille."""
    z = TOP + 0.0055
    case = bevel(box("boitier", (0.05, 0.044, 0.012), flat_material("bracelet_boitier", CASE),
                     location=(0, 0, z)), 0.0018)
    two_tone(case, CASE, CASE_DARK, "bracelet_boitier", dark_side=(0.2, 0.6, -0.7), threshold=0.1,
             seed=5)
    screen = box("ecran", (0.036, 0.03, 0.0012), flat_material("bracelet_ecran", SCREEN, emission=0.6),
                 location=(-0.002, 0, z + 0.0062))
    glow = flat_material("bracelet_lueur", GLOW, emission=3.0)
    lines = [box(f"ligne_{i}", (length, 0.0022, 0.0006), glow,
                 location=(-0.002 - (0.024 - length) / 2, 0.009 - i * 0.0075, z + 0.007))
             for i, length in enumerate((0.024, 0.016, 0.02))]
    lens = cylinder("lentille", 0.0032, 0.003, glow, vertices=8, location=(0.0225, 0, z + 0.0035),
                    rotation_deg=(0, 90, 0))
    return [case, screen, *lines, lens]


def _solar_panels():
    """Cellules solaires sur les deux flancs du brassard, inclinées comme lui."""
    parts = []
    cells = flat_material("bracelet_solaire", SOLAR)
    lines = flat_material("bracelet_solaire_ligne", SOLAR_LINES)
    for side in (-1, 1):
        angle = side * math.radians(62)
        _, y, z = _ring_point(angle, THICKNESS + 0.0008, 0)
        tilt = (-math.degrees(angle), 0, 0)
        parts.append(box(f"solaire_{side}", (0.034, 0.016, 0.0014), cells, location=(0, y, z),
                         rotation_deg=tilt))
        for k in (-1, 0, 1):
            parts.append(box(f"solaire_ligne_{side}_{k}", (0.0007, 0.016, 0.0016), lines,
                             location=(k * 0.0115, y, z), rotation_deg=tilt))
    return parts


def _data_port():
    """Port de données sur le flanc du boîtier (côté pouce), cerclé d'orange."""
    z = TOP + 0.0055
    slot = box("port", (0.011, 0.002, 0.0042), flat_material("bracelet_port", LINER),
               location=(0.008, -0.0222, z))
    ring = box("port_repere", (0.0142, 0.0012, 0.0068), flat_material("bracelet_port_repere",
               PORT_RING), location=(0.008, -0.0217, z))
    return [ring, slot]


def _weld():
    """Soudure grossière sous le poignet : le bracelet ne s'enlève pas."""
    weld = flat_material("bracelet_soudure", WELD)
    beads = []
    for i in range(7):
        x = -HALF_WIDTH + 0.003 + i * (2 * HALF_WIDTH - 0.006) / 6
        beads.append(blob(f"soudure_{i}", 0.0032, weld, subdivisions=1,
                          scale=(1.0, 1.3, 0.7), location=(x, 0.0, -(INNER[0] + THICKNESS + 0.0005))))
    return beads


def build():
    return [_cuff(), *_case(), *_solar_panels(), *_data_port(), *_weld()]
