"""Construit un ou plusieurs modèles d'Odyssée dans Blender, sans ouvrir de fenêtre.

Usage (depuis la racine du dépôt) :
    blender -b --python art/blender/build.py -- hache            # un modèle
    blender -b --python art/blender/build.py -- hache sapin loup # plusieurs
    blender -b --python art/blender/build.py -- --all            # tous

Chaque modèle est un fichier art/blender/modeles/<catégorie>/<nom>.py qui définit build()
et renvoie la liste de ses pièces (et, s'il le veut, VIEW : la direction de la caméra d'aperçu,
pour le montrer sous le même angle que son concept ; ICON = True pour un objet d'inventaire ;
EXPORT = False pour un repère qui reste dans Blender, sans .fbx).
Ce script s'occupe du reste : assemblage, pivot au pied, .blend, .fbx (art/exports/), aperçu
(art/blender/apercus/) et icône d'inventaire (art/exports/icones/).
"""
import importlib.util
import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from lowpoly.export import export_model, save_blend, save_palette  # noqa: E402
from lowpoly.palette import bake  # noqa: E402
from lowpoly.finish import dimensions, join, origin_at_base  # noqa: E402
from lowpoly.outline import ink_outline  # noqa: E402
from lowpoly.preview import DEFAULT_VIEW, render_icon, render_preview  # noqa: E402
from lowpoly.scene import reset_scene  # noqa: E402

MODELS = HERE / "modeles"
ICONS = HERE.parent / "exports" / "icones"


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


def _tilt_for_preview(model, rotation_deg):
    """Incline le modèle comme sur son concept, pour l'aperçu seulement (après l'export)."""
    model.rotation_euler = tuple(math.radians(a) for a in rotation_deg)


def build_one(path):
    category, name = path.parent.name, path.stem
    reset_scene()
    module = load_module(path)
    model = origin_at_base(join(module.build(), name))
    if getattr(module, "OUTLINE", True):  # contour noir des planches (désactivable par modèle)
        ink_outline(model, getattr(module, "OUTLINE_THICKNESS", 0.0035))
    if not getattr(module, "EXPORT", True):  # repère pour Blender seulement (ex. le squelette)
        blend = save_blend(category, name)
        print(f"[odyssee] {category}/{name} : {dimensions(model)} m -> {blend.name} (sans export)")
        return
    save_palette(bake(model, name), category, name)  # un seul matériau (deux si lumineux)
    blend, fbx = export_model(model, category, name)
    view = getattr(module, "VIEW", DEFAULT_VIEW)
    _tilt_for_preview(model, getattr(module, "PREVIEW_ROTATION_DEG", (0, 0, 0)))
    preview = render_preview(model, HERE / "apercus" / f"{name}.png", view=view,
                             color_type="TEXTURE")
    if getattr(module, "ICON", False):  # objet d'inventaire : une icône PNG transparente
        ICONS.mkdir(parents=True, exist_ok=True)
        render_icon(model, ICONS / f"{name}.png", view=getattr(module, "ICON_VIEW", view))
    print(f"[odyssee] {category}/{name} : {dimensions(model)} m, "
          f"{len(model.data.polygons)} faces -> {fbx.name}, {preview.name}")


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not args:
        raise SystemExit("Préciser un ou plusieurs modèles, ou --all.")
    for path in find_models(args):
        build_one(path)


main()
