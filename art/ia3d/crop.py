#!/usr/bin/env python3
"""Découpe chaque objet de models.json dans sa planche : une image carrée par objet, avec marge.

Usage : python3 art/ia3d/crop.py [noms…]      (sans nom : tous)
Sortie : art/ia3d/crops/<nom>.png
"""
import json
import sys
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
MARGIN = 0.08  # marge autour de l'objet, en proportion de sa taille
BACKGROUND = (93, 107, 128)  # le gris-bleu des planches, pour combler les bords


def load_models():
    data = json.loads((HERE / "models.json").read_text(encoding="utf-8"))
    return {name: spec for name, spec in data.items() if not name.startswith("_")}


def square_crop(image, box):
    """Découpe la boîte, ajoute une marge et la place au centre d'une image carrée."""
    left, top, right, bottom = box
    piece = image.crop((left, top, right, bottom))
    side = int(max(piece.size) * (1 + 2 * MARGIN))
    square = Image.new("RGB", (side, side), BACKGROUND)
    square.paste(piece, ((side - piece.width) // 2, (side - piece.height) // 2))
    return square


def crop(name, spec):
    image = Image.open(ROOT / spec["source"]).convert("RGB")
    path = HERE / "crops" / f"{name}.png"
    path.parent.mkdir(exist_ok=True)
    square_crop(image, spec["box"]).save(path)
    return path


def main():
    models = load_models()
    names = sys.argv[1:] or list(models)
    for name in names:
        print(f"{name} -> {crop(name, models[name])}")


if __name__ == "__main__":
    main()
