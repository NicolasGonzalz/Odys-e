"""Mesure un outil dont la tête en métal enjambe le manche (pioche, marteau…), par la COULEUR.

Le métal est gris (rouge ≈ bleu), le bois est orangé (rouge nettement au-dessus du bleu) :
la couleur sépare la tête du manche là où la forme seule ne suffit pas.
  1. détourage, puis seul le plus grand morceau d'un seul tenant est gardé (pas le couteau voisin) ;
  2. axe du manche et redressement de l'image ; séparation métal / bois par la forme et la couleur ;
  3. manche : centre et largeur du bois, ligne par ligne ;
  4. tête : contour du métal colonne par colonne (bord haut, bord bas) ;
  5. couleurs : métal clair / sombre, bois clair / sombre (deux tons, comme sur le dessin).
"""
from collections import deque

import numpy as np

import trace as base



def largest_component(mask):
    """Garde le plus grand morceau d'un seul tenant (parcours en largeur, 8 voisins)."""
    labels = np.zeros(mask.shape, np.int32)
    best_label, best_size, current = 0, 0, 0
    for start in zip(*np.nonzero(mask)):
        if labels[start]:
            continue
        current += 1
        size, queue = 0, deque([start])
        labels[start] = current
        while queue:
            r, c = queue.popleft()
            size += 1
            for dr in (-1, 0, 1):
                for dc in (-1, 0, 1):
                    nr, nc = r + dr, c + dc
                    inside = 0 <= nr < mask.shape[0] and 0 <= nc < mask.shape[1]
                    if inside and mask[nr, nc] and not labels[nr, nc]:
                        labels[nr, nc] = current
                        queue.append((nr, nc))
        if size > best_size:
            best_label, best_size = current, size
    return labels == best_label


def classify(pixels, mask):
    """Masques métal et bois, sur l'image redressée (manche vertical), par la forme ET la couleur.

    - Lignes larges du haut (la tête : pointes et collier) : du métal, même les reflets chauds.
    - Lignes étroites (le manche, et son bout qui dépasse au-dessus) : du bois, même les bords
      mélangés au fond bleuté.
    """
    extents = base.row_extents(mask)
    widths = np.array([r - l for _, l, r in extents])
    handle_width = float(np.median(widths[len(widths) // 2:]))

    metal = np.zeros_like(mask)
    for (row, left, right), width in zip(extents, widths):
        if width <= base.HEAD_WIDTH_RATIO * handle_width:
            continue  # ligne du manche
        metal[row, left:right + 1] = mask[row, left:right + 1]  # pointes ET collier
    return metal, mask & ~metal


def _residual_tilt(mask):
    """Inclinaison restante du manche (degrés) : droite passant par le centre des lignes étroites
    de la moitié basse. Une grosse tête fausse l'axe principal ; le manche, lui, ne ment pas."""
    extents = base.row_extents(mask)
    lower = extents[len(extents) // 2:]
    width = np.median([r - l for _, l, r in lower])
    narrow = [(row, (l + r) / 2) for row, l, r in lower if r - l <= 1.4 * width]
    rows, centers = np.array(narrow, dtype=np.float64).T
    slope = np.polyfit(rows, centers, 1)[0]  # colonnes par ligne
    return float(np.degrees(np.arctan(slope)))


def _tilt_after(pixels, mask, angle):
    return _residual_tilt(base.rotate(pixels, mask, angle)[1])


def refined_angle(pixels, mask, passes=4):
    """Redresse, mesure ce qui penche encore, corrige. À chaque passe, on essaie la correction
    dans les deux sens et on garde celle qui redresse vraiment le manche (pas de pari sur le
    sens de rotation de la bibliothèque d'images)."""
    angle = base.handle_angle(mask)
    for _ in range(passes):
        tilt = _tilt_after(pixels, mask, angle)
        if abs(tilt) < 0.2:
            break
        step = max(-15.0, min(15.0, tilt))
        angle = min((angle + step, angle - step), key=lambda a: abs(_tilt_after(pixels, mask, a)))
    return angle


def _head_columns(metal):
    """Pour chaque colonne de métal : (colonne, ligne du haut, ligne du bas)."""
    columns = []
    for col in range(metal.shape[1]):
        rows = np.nonzero(metal[:, col])[0]
        if len(rows) > 0:
            columns.append((col, int(rows.min()), int(rows.max())))
    return columns


def _two_tones(samples):
    """Ton clair et ton sombre d'une matière (moyennes des moitiés claire et sombre)."""
    lum = np.array([base.luminance(c) for c in samples])
    median = np.median(lum)
    light = samples[lum >= median].mean(axis=0) / 255
    dark = samples[lum < median].mean(axis=0) / 255
    return np.round(light, 3).tolist(), np.round(dark, 3).tolist()


def measure(name, height_m):
    original = base.load_reference(name)
    original_mask = largest_component(base.object_mask(original))
    angle = refined_angle(original, original_mask)
    pixels, mask = base.rotate(original, original_mask, angle)
    metal, wood = classify(pixels, mask)

    rows = np.nonzero(mask.any(axis=1))[0]
    top, bottom = int(rows.min()), int(rows.max())
    scale = height_m / (bottom - top)
    handle_rows = base.row_extents(wood)
    axis = float(np.median([(l + r) / 2 for _, l, r in handle_rows]))

    def to_m(row, col):
        return round((col - axis) * scale, 4), round((bottom - row) * scale, 4)

    handle = []
    for row, left, right in handle_rows[::2]:
        x, z = to_m(row, (left + right) / 2)
        radius = max(1.0, (right - left) / 2 - base.OUTLINE_PX) * scale
        handle.append({"x": x, "z": z, "r": round(radius, 4), "matiere": "bois"})

    columns = _head_columns(metal)
    top_edge = [to_m(r_top, col) for col, r_top, _ in columns]
    bottom_edge = [to_m(r_bottom, col) for col, _, r_bottom in reversed(columns)]

    metal_light, metal_dark = _two_tones(pixels[metal & (np.apply_along_axis(base.luminance, 2, pixels) > 70)])
    wood_light, wood_dark = _two_tones(pixels[wood & (np.apply_along_axis(base.luminance, 2, pixels) > 70)])
    data = {
        "nom": name, "mode": "couleur", "angle_deg": round(angle, 2), "metre_par_pixel": round(scale, 5),
        "manche": handle,
        "tete_contour": [list(p) for p in top_edge + bottom_edge],
        "couleurs": {"metal": metal_light, "metal_sombre": metal_dark,
                     "bois": wood_light, "bois_sombre": wood_dark},
    }
    return data, (pixels, mask, metal, wood)


def draw_control(data, debug, path, zoom=4):
    """Contrôle : métal en magenta, bois en orange, sur l'image redressée."""
    from PIL import Image
    pixels, _, metal, wood = debug
    image = pixels.copy()
    image[metal] = image[metal] * 0.5 + np.array([255, 0, 255]) * 0.5
    image[wood] = image[wood] * 0.5 + np.array([255, 140, 0]) * 0.5
    Image.fromarray(image.astype(np.uint8)).resize(
        (pixels.shape[1] * zoom, pixels.shape[0] * zoom), Image.NEAREST).save(path)
