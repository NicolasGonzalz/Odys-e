#!/usr/bin/env python3
"""Visuels de l'interface (art/ui/), dans le style low-poly du jeu : panneaux et boutons aux coins
coupés, faces légèrement facettées, icônes en polygones nets.

Tout est généré (graine fixe) : relancer ce script redonne les mêmes images. Les icônes d'outils
sont tirées des rendus Blender (art/blender/apercus/), détourés. Le fond du menu principal est une
ambiance de la zone de départ (art/concepts/ambiances/), recadrée.

Ensuite, dans l'éditeur Unreal : tools/ui/import_ui_art.py.

Usage (depuis la racine du dépôt) : python3 tools/ui/generate_ui_art.py
"""
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "art" / "ui"
PREVIEWS = ROOT / "art" / "blender" / "apercus"
AMBIANCE = ROOT / "art" / "concepts" / "ambiances" / "zone-1-plaine.jpg"
SUPERSAMPLE = 4  # dessin en grand puis réduction : bords nets mais lissés
TOOLS = ["hache_pierre", "pioche_pierre", "lance", "lampe", "baton"]


# --- Formes ------------------------------------------------------------------------------------

def chamfered(width, height, cut):
    """Rectangle aux quatre coins coupés à 45° : la silhouette de base de l'interface."""
    return [(cut, 0), (width - cut, 0), (width, cut), (width, height - cut),
            (width - cut, height), (cut, height), (0, height - cut), (0, cut)]


def _facets(draw, width, height, base, spread, rng, cells=3):
    """Triangles de teintes voisines sur toute la surface : l'aspect « facettes »."""
    xs = [width * i / cells for i in range(cells + 1)]
    ys = [height * j / cells for j in range(cells + 1)]
    jitter = lambda v, step: v + rng.uniform(-step, step) * 0.3  # noqa: E731
    points = [[(jitter(x, width / cells) if 0 < i < cells else x,
                jitter(y, height / cells) if 0 < j < cells else y)
               for i, x in enumerate(xs)] for j, y in enumerate(ys)]
    for j in range(cells):
        for i in range(cells):
            a, b = points[j][i], points[j][i + 1]
            c, d = points[j + 1][i], points[j + 1][i + 1]
            for triangle in ((a, b, d), (a, d, c)):
                shade = rng.randint(-spread, spread)
                color = tuple(max(0, min(255, v + shade)) for v in base[:3]) + (base[3],)
                draw.polygon(triangle, fill=color)


def plate(width, height, cut, fill, border, border_px=2, seed=1, spread=7):
    """Plaque facettée aux coins coupés, avec un liseré."""
    s = SUPERSAMPLE
    w, h = width * s, height * s
    shape = chamfered(w - 1, h - 1, cut * s)
    faces = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    _facets(ImageDraw.Draw(faces), w, h, fill, spread, random.Random(seed))
    mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(mask).polygon(shape, fill=255)
    image = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    image.paste(faces, (0, 0), mask)
    ImageDraw.Draw(image).line(shape + [shape[0]], fill=border, width=border_px * s, joint="curve")
    return image.resize((width, height), Image.LANCZOS)


CYAN = (120, 230, 255)


def _holo_fill(w, h, alpha):
    """Fond bleu nuit translucide, un peu plus clair vers le bas. (Les lignes de balayage sont une
    texture répétée à part, holo_lignes : étirées avec la plaque, elles deviendraient des bandes.)"""
    fill = Image.new("RGBA", (w, h))
    draw = ImageDraw.Draw(fill)
    for y in range(h):
        t = y / max(1, h - 1)
        draw.line([(0, y), (w, y)], fill=(int(6 + 6 * t), int(22 + 14 * t), int(34 + 18 * t), alpha))
    return fill


def _brackets(draw, w, h, cut, length, width, color):
    """Équerres lumineuses aux quatre coins coupés : l'accent d'interface « projetée »."""
    for (x0, y0, dx, dy) in ((0, 0, 1, 1), (w - 1, 0, -1, 1), (0, h - 1, 1, -1), (w - 1, h - 1, -1, -1)):
        corner = [(x0 + dx * cut + dx * length, y0), (x0 + dx * cut, y0), (x0, y0 + dy * cut),
                  (x0, y0 + dy * cut + dy * length)]
        draw.line(corner, fill=color, width=width, joint="curve")


def holo_plate(width, height, cut, fill_alpha, edge_alpha, brackets=False, glow=True):
    """Plaque d'hologramme (bracelet de stase) : fond sombre translucide (lisible sur un ciel clair),
    bord cyan lumineux avec un halo intérieur, équerres aux coins pour les panneaux."""
    s = SUPERSAMPLE
    w, h = width * s, height * s
    shape = chamfered(w - 1, h - 1, cut * s)
    mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(mask).polygon(shape, fill=255)

    image = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    image.paste(_holo_fill(w, h, fill_alpha), (0, 0), mask)
    if glow:  # halo : le bord flouté, gardé à l'intérieur de la plaque
        halo = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        ImageDraw.Draw(halo).line(shape + [shape[0]], fill=CYAN + (edge_alpha,), width=6 * s, joint="curve")
        halo = halo.filter(ImageFilter.GaussianBlur(4 * s))
        clipped = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        clipped.paste(halo, (0, 0), mask)
        image = Image.alpha_composite(image, clipped)
    draw = ImageDraw.Draw(image)
    draw.line(shape + [shape[0]], fill=CYAN + (edge_alpha,), width=2 * s, joint="curve")
    if brackets:
        _brackets(draw, w, h, cut * s, 10 * s, 3 * s, (210, 250, 255, 255))
    return image.resize((width, height), Image.LANCZOS)


def holo_sweep(width=256, height=64):
    """Bande de balayage : elle descend lentement sur l'hologramme (fondue en haut et en bas)."""
    image = Image.new("RGBA", (width, height))
    draw = ImageDraw.Draw(image)
    for y in range(height):
        t = 1 - abs(y / (height - 1) * 2 - 1)
        draw.line([(0, y), (width, y)], fill=CYAN + (int(70 * t * t),))
    return image


def holo_lines():
    """Motif des lignes de balayage, répété sur tout l'hologramme : une ligne claire toutes les 3 px."""
    image = Image.new("RGBA", (8, 3), (0, 0, 0, 0))
    ImageDraw.Draw(image).line([(0, 0), (8, 0)], fill=CYAN + (34,))
    return image


def holo_plates():
    return {
        "holo_panneau": holo_plate(128, 128, 18, 175, 230, brackets=True),
        "holo_case": holo_plate(96, 96, 12, 110, 130, glow=False),
        "holo_case_active": holo_plate(96, 96, 12, 150, 255),
        "holo_bouton": holo_plate(256, 64, 14, 150, 170, glow=False),
        "holo_bouton_survol": holo_plate(256, 64, 14, 170, 255),
        "holo_bouton_appui": holo_plate(256, 64, 14, 200, 255),
        "holo_bouton_grise": holo_plate(256, 64, 14, 90, 70, glow=False),
        "holo_barre_fond": holo_plate(256, 20, 6, 140, 150, glow=False),
        "holo_champ": holo_plate(256, 48, 10, 170, 200, glow=False),
        "holo_balayage": holo_sweep(),
        "holo_lignes": holo_lines(),
    }


# --- Icônes (blanches : teintées par l'interface) ---------------------------------------------

ICONS = {  # polygones dans un carré de 64, facettes en niveaux de gris
    # (icone_soif.png, une goutte, est dessinée à la main : le script ne la génère pas)
    "icone_sante": [[(32, 58), (6, 30), (6, 16), (16, 8), (32, 18)],
                    [(32, 58), (58, 30), (58, 16), (48, 8), (32, 18)]],
    "icone_endurance": [[(38, 4), (14, 36), (30, 36), (24, 60)], [(24, 60), (50, 26), (34, 26), (38, 4)]],
    "icone_faim": [[(30, 6), (50, 8), (60, 24), (40, 24)],
                   [(30, 6), (40, 24), (60, 24), (54, 40), (38, 44), (24, 34), (22, 18)],
                   [(26, 36), (34, 42), (18, 56), (12, 50)],
                   [(6, 48), (14, 46), (18, 54), (16, 62), (8, 60), (4, 54)]],
}
FACET_GRAYS = [255, 222]


def icon(polygons, size=64):
    s = SUPERSAMPLE
    image = Image.new("RGBA", (size * s, size * s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    for index, polygon in enumerate(polygons):
        gray = FACET_GRAYS[index % len(FACET_GRAYS)]
        draw.polygon([(x * s, y * s) for x, y in polygon], fill=(gray, gray, gray, 255))
    return image.resize((size, size), Image.LANCZOS)


def tool_icon(name, size=128):
    """Rendu Blender détouré : le fond gris uni est retiré par remplissage depuis les bords."""
    image = Image.open(PREVIEWS / f"{name}.png").convert("RGBA")
    key = (255, 0, 255, 255)
    for corner in ((0, 0), (image.width - 1, 0), (0, image.height - 1),
                   (image.width - 1, image.height - 1)):
        ImageDraw.floodfill(image, corner, key, thresh=28)
    pixels = image.load()
    alpha = Image.new("L", image.size, 255)
    alpha_pixels = alpha.load()
    for y in range(image.height):
        for x in range(image.width):
            if pixels[x, y] == key:
                alpha_pixels[x, y] = 0
    image.putalpha(alpha.filter(ImageFilter.MinFilter(3)))  # retire le halo gris du contour
    image = image.crop(image.getbbox())
    if max(image.size) > 1.6 * min(image.size):  # outil long : en diagonale, il remplit la case
        image = image.rotate(-40 if image.height > image.width else 35, expand=True,
                             resample=Image.BICUBIC)
        image = image.crop(image.getbbox())
    side = max(image.size) + 40
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    square.paste(image, ((side - image.width) // 2, (side - image.height) // 2), image)
    return square.resize((size, size), Image.LANCZOS)


def menu_background():
    """L'ambiance de la plaine, sans le bas (l'image contient déjà une maquette de HUD)."""
    image = Image.open(AMBIANCE).convert("RGB")
    image = image.crop((0, 0, image.width, int(image.height * 0.84)))
    return image.resize((1920, int(1920 * image.height / image.width)), Image.LANCZOS)


# --- Fichiers ----------------------------------------------------------------------------------

def plates():
    return {
        "panneau": plate(128, 128, 18, (22, 27, 22, 228), (196, 186, 150, 170), seed=1),
        "bouton": plate(256, 64, 14, (30, 37, 30, 235), (140, 150, 118, 170), seed=2),
        "bouton_survol": plate(256, 64, 14, (62, 76, 44, 245), (232, 178, 92, 240), seed=2),
        "bouton_appui": plate(256, 64, 14, (96, 60, 30, 250), (240, 190, 105, 255), seed=2),
        "bouton_grise": plate(256, 64, 14, (28, 30, 28, 150), (90, 92, 86, 120), seed=2, spread=3),
        "case": plate(96, 96, 12, (18, 22, 18, 205), (150, 150, 128, 150), seed=3),
        "case_active": plate(96, 96, 12, (52, 60, 40, 225), (236, 186, 96, 255), border_px=3, seed=3),
        "barre_fond": plate(256, 20, 6, (8, 10, 8, 210), (120, 120, 100, 140), border_px=1, seed=4),
        "barre_plein": plate(256, 20, 6, (240, 240, 240, 255), (255, 255, 255, 255), border_px=1,
                             seed=5, spread=18),
        "champ": plate(256, 48, 10, (10, 12, 10, 230), (150, 150, 128, 170), seed=6, spread=3),
    }


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name, image in {**plates(), **holo_plates()}.items():
        image.save(OUT / f"{name}.png")
    for name, polygons in ICONS.items():
        icon(polygons).save(OUT / f"{name}.png")
    for name in TOOLS:
        tool_icon(name).save(OUT / f"outil_{name}.png")
    menu_background().save(OUT / "menu_fond.jpg", quality=90)
    print(f"Interface : {len(list(OUT.iterdir()))} images dans {OUT}")


if __name__ == "__main__":
    main()
