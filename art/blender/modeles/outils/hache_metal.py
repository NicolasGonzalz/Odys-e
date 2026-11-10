"""Hache en métal (niveau 2), reconstruite à partir des MESURES de son image de concept.

Référence : art/concepts/objets/outils-et-ressources.png (découpe : references/hache.png)
Mesures : references/hache_trace.json (python3 art/blender/references/trace.py hache)
Modèle debout (manche selon z), tranchant vers -x comme sur le concept. Hauteur 0,75 m.

Ce qui vient de l'image : la courbe et la largeur du manche, la position des bandages, du bois et
de l'embout, les taches de rouille et toutes les couleurs. La tête est redessinée (HEAD) d'après le
concept : le contour mesuré donnait un pavé trop épais et sans forme.
"""
import json
from pathlib import Path

from lowpoly import handle
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.shapes import cylinder, roughen
from lowpoly.tool_parts import HAFT_FLATTEN, HAFT_SIDES, TOOL_COLORS, shade_wood, wood_details
from lowpoly.wraps import crossed_wrap, frayed_edge

TRACE = Path(__file__).resolve().parents[2] / "references" / "hache_trace.json"

ICON = True
VIEW = (0.0, -1.0, 0.12)              # de face, comme le dessin
PREVIEW_ROTATION_DEG = (0, 24, 0)     # incliné comme sur le concept (aperçu seulement)

POLL_THICKNESS = 0.044      # épaisseur à l'œil (m)
EDGE_BAND = 0.018           # largeur de la bande de métal clair le long du tranchant (m)
RUST_GROUPING = 0.02        # deux taches à moins de 2 cm forment une seule plaque (m)


def _load():
    return json.loads(TRACE.read_text(encoding="utf-8"))


def _material(name, colors):
    return flat_material(f"hache_{name}", tuple(colors[name]))


def _lighter(color, factor=1.22):
    return tuple(min(1.0, c * factor) for c in color)


def _darker(color, factor=0.78):
    return tuple(c * factor for c in color)


# --- Manche --------------------------------------------------------------------------------------

EYE_Z = (0.592, 0.718)   # bas et haut de l'œil de la tête (où le manche est caché)


NECK_RADIUS = 0.0215     # sous la tête, le manche s'affine jusqu'à l'œil


def _handle_slices(trace):
    """Tranches du manche ; juste sous la tête, le rayon mesuré (gonflé par l'ancienne tête du
    dessin, qui débordait) est ramené vers la taille de l'œil."""
    slices = handle.prepared(trace, EYE_Z[0], EYE_Z[1], POLL_THICKNESS / 2 * 0.8)
    for slice_ in slices:
        if slice_["z"] > EYE_Z[0] - 0.09:
            t = min(1.0, (slice_["z"] - (EYE_Z[0] - 0.09)) / 0.09)
            slice_["r"] = min(slice_["r"], slice_["r"] + (NECK_RADIUS - slice_["r"]) * t)
    return slices


def _handle(slices, colors):
    """Un tube par matière ; le bois et l'embout ont un côté clair et un côté sombre."""
    parts = handle.build("manche", slices, lambda m: _material(m, colors), sides=HAFT_SIDES,
                         flatten=HAFT_FLATTEN)
    for part in parts:
        material = part.name.rsplit("_", 1)[-1]
        if material == "bois":
            shade_wood(part, seed=1)
        elif material == "embout":
            two_tone(part, colors[material], _darker(colors[material]), f"hache_{material}")
    return parts


def _wood_grain(slices, center_x, radius_at, below_z):
    """Fil, nœud du bois sur chaque partie de bois nu visible (pas celle cachée par la tête).
    Le pied est couvert par l'embout : pas de bout scié."""
    runs = [[s for s in run if s["z"] < below_z] for run in handle.runs(slices)
            if run[-1]["matiere"] == "bois"]
    details = []
    for index, run in enumerate(r for r in runs if len(r) >= 2):
        details += wood_details(f"manche_{index}", center_x, radius_at, run[0]["z"] + 0.01,
                                run[-1]["z"] - 0.01, seed=index + 1, sawn_end=False)
    return details


def _wraps(slices, center_x, radius_at, colors):
    """Bandage du haut croisé en X ; bandage du bas avec des franges déchirées."""
    cloth = _material("tissu", colors)
    cloth_runs = sorted((run for run in handle.runs(slices) if run[-1]["matiere"] == "tissu"),
                        key=lambda run: -run[0]["z"])
    parts = []
    for index, run in enumerate(cloth_runs):
        z0, z1 = run[0]["z"], run[-1]["z"]
        sleeve = [(center_x(z), 0.0, z) for z in (z0, z1)]
        parts.append(tube(f"bandage_{index}", sleeve, [radius_at(z0) + 0.002, radius_at(z1) + 0.002],
                          cloth, sides=7, flatten=HAFT_FLATTEN))
        turns = 1.2 if index == 0 else 0.8
        parts += crossed_wrap(f"bandage_{index}", center_x, radius_at, z0, z1, cloth, turns=turns,
                              flatten=HAFT_FLATTEN)
        if index == len(cloth_runs) - 1:
            parts += frayed_edge(f"bandage_{index}", center_x, radius_at, z0, cloth, seed=index,
                                 flatten=HAFT_FLATTEN)
    return parts


# --- Tête ---------------------------------------------------------------------------------------

# Profil de la tête (x, z), dessiné d'après le concept : tranchant bombé et ébréché, barbe qui
# descend, col affiné vers l'œil, talon compact. Le manche passe à x ≈ 0,02.
HEAD = [(0.075, 0.705), (0.075, 0.6), (0.05, 0.592), (-0.01, 0.59), (-0.045, 0.6), (-0.08, 0.585),
        (-0.115, 0.55), (-0.14, 0.505), (-0.16, 0.472), (-0.178, 0.478), (-0.195, 0.52),
        (-0.207, 0.575), (-0.199, 0.592), (-0.21, 0.605), (-0.212, 0.63), (-0.207, 0.685),
        (-0.198, 0.725), (-0.188, 0.748), (-0.15, 0.742), (-0.1, 0.726), (-0.06, 0.714),
        (-0.03, 0.716), (0.0, 0.722), (0.04, 0.722), (0.05, 0.712)]

# Épaisseur (x, épaisseur totale) : fil fin, joues qui se renflent, œil épais, talon un peu moins.
THICKNESS = [(-0.215, 0.004), (-0.17, 0.008), (-0.12, 0.015), (-0.06, 0.026), (-0.03, 0.044),
             (0.05, 0.044), (0.075, 0.038)]


def _thickness(x):
    """Épaisseur de la tête à l'abscisse x, interpolée dans THICKNESS (en amande vue de dessus)."""
    for (x0, t0), (x1, t1) in zip(THICKNESS, THICKNESS[1:]):
        if x <= x1:
            u = max(0.0, (x - x0) / (x1 - x0))
            return t0 + (t1 - t0) * u
    return THICKNESS[-1][1]


def _head(colors):
    blade_x = min(x for x, _ in HEAD)
    edge_limit = blade_x + EDGE_BAND
    metal = _material("metal", colors)
    edge = flat_material("hache_tranchant", _lighter(colors["metal"]))
    cuts = [x for x, _ in THICKNESS[1:-1]] + [edge_limit, -0.09, 0.02]
    head = profile_solid("tete", HEAD, _thickness, [metal], lambda x: 0, cuts_x=sorted(cuts))
    two_tone(head, colors["metal"], _darker(colors["metal"], 0.85), "hache_metal_tete",
             threshold=0.6)
    head.data.materials.append(edge)  # après les deux tons : le fil reste clair
    for polygon in head.data.polygons:
        if polygon.center.x < edge_limit:
            polygon.material_index = len(head.data.materials) - 1
    return head


def _inside(x, z, outline=HEAD, margin=0.012):
    """Vrai si (x, z) est dans le profil de la tête, à `margin` du bord (rayons horizontaux)."""
    crossings = 0
    for (x0, z0), (x1, z1) in zip(outline, outline[1:] + outline[:1]):
        if (z0 > z) != (z1 > z) and x < x0 + (z - z0) * (x1 - x0) / (z1 - z0):
            crossings += 1
    near = any(abs(x - px) < margin and abs(z - pz) < margin for px, pz in outline)
    return crossings % 2 == 1 and not near and x < 0.045  # pas sur le talon (rivets)


def _rust_patches(trace, colors):
    """Taches mesurées regroupées en plaques irrégulières, sur les deux faces de la tête
    (seulement celles qui tombent dans le profil redessiné)."""
    groups = []
    for spot in (s for s in trace["rouille"] if _inside(s["x"], s["z"])):
        near = next((g for g in groups if abs(g[0]["x"] - spot["x"]) < RUST_GROUPING
                     and abs(g[0]["z"] - spot["z"]) < RUST_GROUPING), None)
        if near is None:
            groups.append([spot])
        else:
            near.append(spot)

    rust = _material("rouille", colors)
    patches = []
    for index, group in enumerate(groups):
        x = sum(s["x"] for s in group) / len(group)
        z = sum(s["z"] for s in group) / len(group)
        radius = min(0.022, 0.009 + 0.004 * len(group))
        half = _thickness(x) / 2 + 0.0008
        for side in (-1, 1):
            patch = cylinder(f"rouille_{index}_{side}", radius, 0.0015, rust, vertices=7,
                             location=(x, side * half, z), rotation_deg=(90, 0, index * 23))
            patches.append(roughen(patch, radius * 0.25, seed=index * 2 + side))
    return patches


def _rivets():
    """Deux rivets sur chaque face du talon : la tête est fixée au manche."""
    dark = flat_material("outil_rivet", TOOL_COLORS["metal_sombre"])
    rivets = []
    for index, z in enumerate((0.62, 0.69)):
        for side in (-1, 1):
            rivets.append(cylinder(f"rivet_{index}_{side}", 0.0055, 0.004, dark, vertices=6,
                                   location=(0.058, side * (_thickness(0.058) / 2 + 0.0012), z),
                                   rotation_deg=(90, 0, 0)))
    return rivets


def build():
    trace = _load()
    shared = {name: TOOL_COLORS[name] for name in ("bois", "tissu", "metal", "rouille")}
    colors = {**trace["couleurs"], **shared}  # palette commune à tous les outils
    slices = _handle_slices(trace)
    center_x, radius_at = handle.interpolator(slices, "x"), handle.interpolator(slices, "r")

    head = _head(colors)
    return (_handle(slices, colors) + _wood_grain(slices, center_x, radius_at, EYE_Z[0])
            + _wraps(slices, center_x, radius_at, colors) + [head]
            + _rust_patches(trace, colors) + _rivets())
