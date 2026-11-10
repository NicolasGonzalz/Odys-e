#!/usr/bin/env python3
"""Mesure un outil à manche (hache, pioche…) directement sur son image de concept.

L'image est analysée pour en tirer la vraie géométrie, que Blender reconstruit ensuite en 3D :
  1. détourage de l'objet (tout ce qui n'est pas la couleur de fond) ;
  2. axe du manche (analyse en composantes principales) et redressement de l'image ;
  3. ligne par ligne : centre et largeur du manche ; là où l'objet s'élargit, c'est la tête ;
  4. matière de chaque partie par sa couleur (bois, tissu, embout ; métal, rouille) ;
  5. couleurs moyennes réelles de chaque matière.

Usage : python3 art/blender/references/trace.py hache [hauteur_m]
Sorties : references/<nom>_trace.json (mesures en mètres) et <nom>_trace.png (contrôle visuel).
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BACKGROUND_DISTANCE = 28   # écart de couleur au-delà duquel un pixel appartient à l'objet
OUTLINE_LUMINANCE = 70     # les contours noirs du dessin
HEAD_WIDTH_RATIO = 1.8     # une ligne plus large que 1,8 × le manche appartient à la tête
POLL_OVERHANG = 0.25       # le talon dépasse à droite d'au moins 0,25 × la largeur du manche (~8 px)
                           # (les bandages, eux, ne dépassent que de 1 ou 2 pixels)
CLOTH_LUMINANCE = 120      # mesuré : tissu 126–146, bois et embout ~90–100
WOOD_RED_MINUS_GREEN = 30  # mesuré : bois ~38, embout ~14
RUST_RED_MINUS_GREEN = 32  # mesuré sur la tête : métal ~13, taches de rouille 41–67
SMOOTHING = 2              # une matière doit se retrouver sur les tranches voisines
OUTLINE_PX = 1.5           # épaisseur du contour noir du dessin, de chaque côté


# --- Image --------------------------------------------------------------------------------------

def load_reference(name):
    spec = json.loads((HERE / "references.json").read_text(encoding="utf-8"))[name]
    image = Image.open(ROOT / spec["source"]).convert("RGB").crop(tuple(spec["box"]))
    return np.asarray(image).astype(np.float32)


def background_color(pixels):
    """Couleur médiane de tout le bord du cadre (un coin peut toucher un objet voisin)."""
    border = np.concatenate([pixels[0], pixels[-1], pixels[:, 0], pixels[:, -1]])
    return np.median(border, axis=0)


def object_mask(pixels):
    return np.linalg.norm(pixels - background_color(pixels), axis=2) > BACKGROUND_DISTANCE


# --- Redressement -------------------------------------------------------------------------------

def handle_angle(mask):
    """Angle du manche (degrés) : axe principal des pixels de la moitié basse de l'objet."""
    rows, cols = np.nonzero(mask)
    lower = rows > np.percentile(rows, 45)
    points = np.stack([cols[lower], rows[lower]], axis=1).astype(np.float64)
    points -= points.mean(axis=0)
    _, vectors = np.linalg.eigh(points.T @ points)
    dx, dy = vectors[:, -1]
    return float(np.degrees(np.arctan2(dx, dy)))


def rotate(pixels, mask, angle_deg):
    """Redresse l'image pour que le manche soit vertical (bords remplis de fond)."""
    fill = tuple(int(c) for c in background_color(pixels))
    image = Image.fromarray(pixels.astype(np.uint8)).rotate(-angle_deg, Image.BICUBIC,
                                                             expand=True, fillcolor=fill)
    mask_image = Image.fromarray((mask * 255).astype(np.uint8)).rotate(-angle_deg, Image.NEAREST,
                                                                       expand=True)
    return np.asarray(image).astype(np.float32), np.asarray(mask_image) > 127


# --- Mesures ligne par ligne --------------------------------------------------------------------

def row_extents(mask):
    """Pour chaque ligne contenant l'objet : (ligne, gauche, droite)."""
    extents = []
    for row in range(mask.shape[0]):
        cols = np.nonzero(mask[row])[0]
        if len(cols) > 0:
            extents.append((row, int(cols.min()), int(cols.max())))
    return extents


def first_run_end(mask_row, start):
    """Fin du morceau continu d'objet qui commence en `start` (ex. la lame, jusqu'au fond)."""
    col = start
    while col + 1 < len(mask_row) and mask_row[col + 1]:
        col += 1
    return col


def split_head(extents, mask):
    """Sépare la tête du manche.

    Le manche est une bande verticale autour de son axe. Sur les lignes larges du haut :
      - si l'objet dépasse à droite de la bande, c'est toute la tête (talon compris) ;
      - sinon, seul ce qui est à gauche de la bande est la tête (la lame qui descend) ;
        la bande, elle, est encore le manche (avec son bandage).
    """
    widths = np.array([right - left for _, left, right in extents])
    handle_width = float(np.median(widths[len(widths) // 2:]))
    lower = extents[len(extents) // 2:]
    axis = float(np.median([(l + r) / 2 for _, l, r in lower]))
    band_left, band_right = axis - handle_width / 2 - 1, axis + handle_width / 2 + 1

    head_rows, handle_rows = [], []
    for (row, left, right), width in zip(extents, widths):
        if width <= HEAD_WIDTH_RATIO * handle_width:
            handle_rows.append((row, left, right))
        elif right > band_right + POLL_OVERHANG * handle_width:
            head_rows.append((row, left, right))  # talon : toute la ligne est en métal
        else:
            # Sous le talon : la lame va de son bord gauche jusqu'au fond (ou jusqu'au manche).
            blade_end = min(first_run_end(mask[row], left), int(band_left))
            head_rows.append((row, left, blade_end))
            handle_rows.append((row, int(band_left), right))
    return head_rows, handle_rows, handle_width, axis


# --- Matières -----------------------------------------------------------------------------------

def luminance(color):
    return 0.299 * color[0] + 0.587 * color[1] + 0.114 * color[2]


def handle_material(color):
    """bois (brun chaud), tissu (clair), embout (olive, plus vert que rouge-orangé)."""
    r, g, b = color
    if luminance(color) > CLOTH_LUMINANCE:
        return "tissu"
    if r - g > WOOD_RED_MINUS_GREEN:
        return "bois"
    return "embout"


def is_rust(color):
    r, g, _ = color
    return luminance(color) > OUTLINE_LUMINANCE and r - g > RUST_RED_MINUS_GREEN


def row_color(pixels, row, left, right):
    """Couleur moyenne du centre d'une ligne, sans les contours noirs."""
    span = pixels[row, left + (right - left) // 4: right - (right - left) // 4 + 1]
    bright = span[[luminance(c) > OUTLINE_LUMINANCE for c in span]]
    return bright.mean(axis=0) if len(bright) else span.mean(axis=0)


def head_spots(pixels, mask, head_rows):
    """Taches de rouille de la tête : pixels orangés, regroupés en petits disques."""
    spots, seen = [], np.zeros(mask.shape, bool)
    for row, left, right in head_rows:
        for col in range(left, right + 1):
            if seen[row, col] or not is_rust(pixels[row, col]):
                continue
            seen[max(0, row - 3):row + 4, max(0, col - 3):col + 4] = True
            spots.append((row, col))
    return spots


# --- Assemblage des mesures ---------------------------------------------------------------------

def measure(name, height_m):
    pixels = load_reference(name)
    mask = object_mask(pixels)
    angle = handle_angle(mask)
    pixels, mask = rotate(pixels, mask, angle)
    extents = row_extents(mask)
    head_rows, handle_rows, _, axis_col = split_head(extents, mask)

    bottom = max(r for r, _, _ in extents)
    top = min(r for r, _, _ in extents)
    scale = height_m / (bottom - top)

    def to_m(row, col):
        return round((col - axis_col) * scale, 4), round((bottom - row) * scale, 4)

    handle = []
    for row, left, right in handle_rows[::2]:
        x, z = to_m(row, (left + right) / 2)
        color = row_color(pixels, row, left, right)
        radius = max(1.0, (right - left) / 2 - OUTLINE_PX) * scale  # sans le contour noir
        handle.append({"x": x, "z": z, "r": round(radius, 4),
                       "matiere": handle_material(color), "couleur": (color / 255).round(3).tolist()})
    smooth_materials(handle)

    head = [{"z": to_m(row, 0)[1], "gauche": to_m(row, left)[0], "droite": to_m(row, right)[0]}
            for row, left, right in head_rows]
    handle_radius = float(np.median([h["r"] for h in handle]))
    spots = [dict(zip(("x", "z"), to_m(row, col))) for row, col in head_spots(pixels, mask, head_rows)]
    spots = [s for s in spots if abs(s["x"]) > handle_radius * 1.3]  # pas sur le bois du manche

    return {
        "nom": name, "angle_deg": round(angle, 2), "metre_par_pixel": round(scale, 5),
        "manche": handle, "tete": head, "rouille": spots,
        "couleurs": _colors(pixels, head_rows, handle),
    }, (pixels, mask, head_rows, handle_rows)


def smooth_materials(handle):
    """Une tranche isolée (ombre, reflet) prend la matière majoritaire de ses voisines."""
    materials = [h["matiere"] for h in handle]
    for i, h in enumerate(handle):
        window = materials[max(0, i - SMOOTHING): i + SMOOTHING + 1]
        h["matiere"] = max(set(window), key=window.count)


def _colors(pixels, head_rows, handle):
    """Couleur moyenne de chaque matière (0 à 1)."""
    metal = [pixels[r, c] for r, l, rr in head_rows for c in range(l, rr + 1)
             if luminance(pixels[r, c]) > 140 and not is_rust(pixels[r, c])]
    rust = [pixels[r, c] for r, l, rr in head_rows for c in range(l, rr + 1) if is_rust(pixels[r, c])]
    colors = {"metal": np.mean(metal, axis=0) / 255, "rouille": np.mean(rust, axis=0) / 255}
    for material in ("bois", "tissu", "embout"):
        samples = [h["couleur"] for h in handle if h["matiere"] == material]
        if samples:
            colors[material] = np.mean(samples, axis=0)
    return {k: np.round(v, 3).tolist() for k, v in colors.items()}


# --- Contrôle visuel ----------------------------------------------------------------------------

def draw_control(data, debug, path, zoom=4):
    """Image de contrôle : matières du manche (orange bois, bleu tissu, vert embout),
    bords de la tête (magenta) et taches de rouille (cercles rouges)."""
    pixels, mask, head_rows, handle_rows = debug
    image = Image.fromarray(pixels.astype(np.uint8)).resize(
        (pixels.shape[1] * zoom, pixels.shape[0] * zoom), Image.NEAREST)
    draw = ImageDraw.Draw(image)
    tints = {"bois": (255, 120, 0), "tissu": (0, 200, 255), "embout": (120, 255, 0)}
    for (row, left, right), slice_ in zip(handle_rows[::2], data["manche"]):
        y = row * zoom
        draw.line([(left * zoom, y), (right * zoom, y)], fill=tints[slice_["matiere"]], width=1)
    for row, left, right in head_rows:
        draw.point([(left * zoom, row * zoom), (right * zoom, row * zoom)], fill=(255, 0, 255))
    for row, col in head_spots(pixels, mask, head_rows):
        draw.ellipse((col * zoom - 6, row * zoom - 6, col * zoom + 6, row * zoom + 6),
                     outline=(255, 0, 0), width=2)
    image.save(path)


def main():
    name = sys.argv[1]
    height = float(sys.argv[2]) if len(sys.argv) > 2 else 0.75
    spec = json.loads((HERE / "references.json").read_text(encoding="utf-8"))[name]
    if spec.get("mode") == "couleur":
        import trace_color as method   # tête qui enjambe le manche (pioche…)
    else:
        method = sys.modules[__name__]  # tête sur un côté (hache…)
    data, debug = method.measure(name, height)
    (HERE / f"{name}_trace.json").write_text(json.dumps(data, indent=1, ensure_ascii=False))
    method.draw_control(data, debug, HERE / f"{name}_trace.png")
    print(f"{name} : angle {data['angle_deg']}°, {len(data['manche'])} tranches de manche ; "
          f"couleurs {data['couleurs']}")


if __name__ == "__main__":
    main()
