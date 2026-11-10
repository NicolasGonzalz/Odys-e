"""Construit un ou plusieurs modèles d'Odyssée dans Blender, sans ouvrir de fenêtre.

Usage (depuis la racine du dépôt) :
    blender -b --python art/blender/build.py -- hache            # un modèle
    blender -b --python art/blender/build.py -- hache sapin loup # plusieurs
    blender -b --python art/blender/build.py -- --all            # tous

Chaque modèle est un fichier art/blender/modeles/<catégorie>/<nom>.py qui définit build()
et renvoie la liste de ses pièces. Ce script s'occupe du reste :
assemblage, pivot au pied, .blend, .fbx (art/exports/) et aperçu (art/blender/apercus/).
"""
import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from lowpoly.export import export_model  # noqa: E402
from lowpoly.finish import dimensions, join, origin_at_base  # noqa: E402
from lowpoly.preview import render_preview  # noqa: E402
from lowpoly.scene import reset_scene  # noqa: E402

MODELS = HERE / "modeles"


def find_models(names):
    files = sorted(MODELS.glob("*/*.py"))
    if names == ["--all"]:
        return files
    found = {f.stem: f for f in files}
    missing = [n for n in names if n not in found]
    if missing:
        raise SystemExit(f"Modèle(s) introuvable(s) : {', '.join(missing)} "
                         f"(disponibles : {', '.join(sorted(found))})")
    return [found[n] for n in names]


def load_module(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_one(path):
    category, name = path.parent.name, path.stem
    reset_scene()
    parts = load_module(path).build()
    model = origin_at_base(join(parts, name))
    blend, fbx = export_model(model, category, name)
    preview = render_preview(model, HERE / "apercus" / f"{name}.png")
    print(f"[odyssee] {category}/{name} : {dimensions(model)} m, "
          f"{len(model.data.polygons)} faces -> {fbx.name}, {preview.name}")


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not args:
        raise SystemExit("Préciser un ou plusieurs modèles, ou --all.")
    for path in find_models(args):
        build_one(path)


main()
