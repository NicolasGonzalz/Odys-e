#!/usr/bin/env python3
"""Planche de comparaison : la référence (concept) à gauche, le rendu du modèle à droite.

Usage : python3 art/blender/references/compare.py <nom> [noms…]
Sortie : art/blender/apercus/<nom>_comparaison.png
"""
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
PREVIEWS = HERE.parent / "apercus"
HEIGHT = 600


def _fit(image):
    ratio = HEIGHT / image.height
    return image.convert("RGB").resize((round(image.width * ratio), HEIGHT))


def _reference_for(name):
    """Image de référence d'un modèle : la sienne, ou celle qu'il partage (clé « modeles » de
    references.json : la découpe « hache » sert à hache_metal)."""
    data = json.loads((HERE / "references.json").read_text(encoding="utf-8"))
    for ref, spec in data.items():
        if name in spec.get("modeles", []) if isinstance(spec, dict) else False:
            return HERE / f"{ref}.png"
    return HERE / f"{name}.png"


def compare(name):
    reference = _fit(Image.open(_reference_for(name)))
    render = _fit(Image.open(PREVIEWS / f"{name}.png"))
    sheet = Image.new("RGB", (reference.width + render.width + 10, HEIGHT + 30), (40, 40, 45))
    sheet.paste(reference, (0, 30))
    sheet.paste(render, (reference.width + 10, 30))
    draw = ImageDraw.Draw(sheet)
    draw.text((10, 8), f"{name} — concept", fill=(230, 230, 230))
    draw.text((reference.width + 20, 8), "modèle Blender", fill=(230, 230, 230))
    path = PREVIEWS / f"{name}_comparaison.png"
    sheet.save(path)
    return path


if __name__ == "__main__":
    for model in sys.argv[1:]:
        print(compare(model))
