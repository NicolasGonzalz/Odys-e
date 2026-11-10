"""Pioche en métal (niveau 2), reconstruite à partir des MESURES de son image de concept.

Référence : art/concepts/objets/outils-et-ressources.png (découpe : references/pioche.png)
Mesures : references/pioche_trace.json (python3 art/blender/references/trace.py pioche 0.85)
  - manche : centre et largeur à chaque hauteur (bois nu, deux tons) ;
  - tête : contour du métal colonne par colonne (deux pointes et un collier) ; couleurs réelles.
Modèle debout (manche selon z). Hauteur 0,85 m.

Détails ajoutés dans le style commun des outils (lowpoly/tool_parts.py) : collier facetté à huit pans,
cale sur le dessus, pointes usées plus claires, rouille, poignée bandée.
"""
import json
from pathlib import Path

from lowpoly import handle
from lowpoly.materials import flat_material
from lowpoly.profile import profile_solid, simplify
from lowpoly.shading import two_tone
from lowpoly.shapes import bevel, box, cylinder
from lowpoly.tool_parts import (HAFT_FLATTEN, HAFT_SIDES, TOOL_COLORS, grip, material, rust_spots,
                                shade_wood, wood_details)

TRACE = Path(__file__).resolve().parents[2] / "references" / "pioche_trace.json"

ICON = True
VIEW = (0.0, -1.0, 0.12)              # de face, comme le dessin
PREVIEW_ROTATION_DEG = (0, 23, 0)     # incliné comme sur le concept (aperçu seulement)

COLLAR_THICKNESS = 0.052   # le collier enserre le manche (m)
ARM_ROOT = 0.034           # épaisseur des pointes au départ du collier (m)
TIP_THICKNESS = 0.007      # épaisseur au bout des pointes (m)
OUTLINE_TOLERANCE = 0.009  # simplification du contour : ~2 pixels de l'image (1 px ≈ 4,5 mm)
COLLAR_RATIO = 1.4         # une colonne plus haute que 1,4 × la médiane appartient au collier
WORN_TIP = 0.035           # longueur du bout des pointes, plus clair (métal usé), en m


def _load():
    return json.loads(TRACE.read_text(encoding="utf-8"))


# --- Tête ---------------------------------------------------------------------------------------

def _edges(contour):
    """Bord haut et bord bas de la tête, tous deux de gauche à droite (mêmes colonnes)."""
    half = len(contour) // 2
    top = [tuple(p) for p in contour[:half]]
    bottom = [tuple(p) for p in reversed(contour[half:])]
    return top, bottom


def _collar(top, bottom):
    """Étendue (x min, x max, z bas, z haut) du collier : les colonnes nettement plus hautes."""
    heights = [t[1] - b[1] for t, b in zip(top, bottom)]
    median = sorted(heights)[len(heights) // 2]
    columns = [i for i, h in enumerate(heights) if h > COLLAR_RATIO * median]
    xs = [top[i][0] for i in columns]
    return min(xs), max(xs), min(bottom[i][1] for i in columns), max(top[i][1] for i in columns)


def _thickness(x, collar, tips):
    """Épaisse au collier, puis chaque pointe s'amincit jusqu'à son bout."""
    left, right = collar[0], collar[1]
    if left <= x <= right:
        return COLLAR_THICKNESS
    tip = tips[0] if x < left else tips[1]
    edge = left if x < left else right
    t = min(1.0, abs(x - edge) / max(abs(tip - edge), 1e-6))
    return ARM_ROOT + (TIP_THICKNESS - ARM_ROOT) * t


def _head(trace, colors):
    top, bottom = _edges(trace["tete_contour"])
    collar = _collar(top, bottom)
    tips = (top[0][0], top[-1][0])
    outline = (simplify(top, OUTLINE_TOLERANCE)
               + list(reversed(simplify(bottom, OUTLINE_TOLERANCE))))
    worn = (tips[0] + WORN_TIP, tips[1] - WORN_TIP)
    metal = flat_material("pioche_metal", tuple(colors["metal"]))
    head = profile_solid("tete", outline, lambda x: _thickness(x, collar, tips), [metal],
                         lambda x: 0, cuts_x=(worn[0], collar[0], collar[1], worn[1]))
    two_tone(head, colors["metal"], colors["metal_sombre"], "pioche_metal", threshold=0.55)
    _brighten_tips(head, worn)
    return head, collar, tips


def _brighten_tips(head, worn):
    """Le bout des pointes, usé par les coups, est plus clair."""
    head.data.materials.append(material("metal_clair"))
    clear = len(head.data.materials) - 1
    for polygon in head.data.polygons:
        if not worn[0] <= polygon.center.x <= worn[1]:
            polygon.material_index = clear


def _collar_block(collar):
    """Collier facetté à huit pans qui enserre le manche (comme sur le concept)."""
    left, right, bottom, top = collar
    radius = (right - left) / 2 * 1.08
    block = cylinder("collier", radius, top - bottom, material("metal_sombre"), vertices=8,
                     location=((left + right) / 2, 0, (bottom + top) / 2), rotation_deg=(0, 0, 22.5))
    bevel(block, 0.004)
    two_tone(block, TOOL_COLORS["metal"], TOOL_COLORS["metal_sombre"], "pioche_collier",
             threshold=0.4)
    return block


def _wedge(collar):
    """Cale en métal enfoncée dans le haut du manche, qui bloque la tête."""
    left, right, _, top = collar
    return box("cale", (0.012, 0.03, 0.012), material("metal_sombre"),
               location=((left + right) / 2, 0, top + 0.03))


def _rust(tips, collar, thickness_at):
    """Quelques taches de rouille sur les pointes."""
    mid_left = (tips[0] + collar[0]) / 2
    mid_right = (tips[1] + collar[1]) / 2
    z = (collar[2] + collar[3]) / 2
    spots = [(mid_left, z + 0.004, 0.012), (mid_left - 0.06, z - 0.004, 0.008),
             (mid_right, z - 0.01, 0.011)]
    return rust_spots("rouille", spots, lambda x: thickness_at(x) / 2, seed=3)


# --- Manche -------------------------------------------------------------------------------------

def _handle(trace, collar):
    """Manche mesuré, au bois commun à tous les outils, avec la poignée bandée commune."""
    slices = handle.prepared(trace, collar[2], collar[3], COLLAR_THICKNESS / 2 * 0.8)
    parts = handle.build("manche", slices, lambda _: material("bois"), sides=HAFT_SIDES,
                         flatten=HAFT_FLATTEN)
    parts = [shade_wood(part, seed=3) for part in parts]
    center_x, radius_at = handle.interpolator(slices, "x"), handle.interpolator(slices, "r")
    details = wood_details("manche", center_x, radius_at, 0.19, collar[2] - 0.03, seed=3)
    return parts + details + grip("poignee", center_x, radius_at, 0.03, 0.17, seed=3)


def build():
    trace = _load()
    colors = {**trace["couleurs"], "metal": TOOL_COLORS["metal"],
              "metal_sombre": TOOL_COLORS["metal_sombre"]}
    head, collar, tips = _head(trace, colors)
    thickness_at = lambda x: _thickness(x, collar, tips)  # noqa: E731
    return (_handle(trace, collar) + [head, _collar_block(collar), _wedge(collar)]
            + _rust(tips, collar, thickness_at))
