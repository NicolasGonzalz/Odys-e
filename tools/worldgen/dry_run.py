#!/usr/bin/env python3
"""Exécute la génération de la carte HORS d'Unreal, pour la vérifier sous Linux.

Calcule exactement le même plan que build_map.py (mêmes fichiers, même graine), puis :
  - affiche le nombre d'éléments par type ;
  - vérifie les règles (rien dans l'eau ni sur les routes, tout dans sa zone) ;
  - dessine output/scatter_preview.png (chaque élément en couleur sur l'aperçu du relief).

Usage : python3 tools/worldgen/dry_run.py   (dépendance : Pillow, pour l'image)
"""
import sys
import time
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "unreal"))

from odyssee_world.catalog import load_catalog, load_rules  # noqa: E402
from odyssee_world.layout import Layout  # noqa: E402
from odyssee_world.plan import build_plan  # noqa: E402
from odyssee_world.terrain import Terrain  # noqa: E402

SCALE = 4  # 1 pixel de l'aperçu = 4 m


def build():
    layout = Layout.load(HERE / "world_layout.json")
    terrain = Terrain(HERE / "output")
    catalog = load_catalog(HERE / "unreal" / "elements.json")
    rules = load_rules(HERE / "unreal" / "scatter_rules.json")
    return layout, terrain, catalog, build_plan(layout, terrain, catalog, rules)


def report(plan):
    print(f"{'élément':<22} {'nombre':>8}")
    for element, items in sorted(plan.instances.items(), key=lambda kv: -len(kv[1])):
        print(f"{element:<22} {len(items):>8}")
    kinds = Counter(actor.tags[0] for actor in plan.labeled)
    print(f"acteurs nommés : {dict(kinds)} ; apparition : {tuple(round(v, 1) for v in plan.spawn)}")
    print(f"total : {plan.count()} éléments")


def check(plan, layout, terrain):
    """Vérifie les règles importantes ; renvoie la liste des problèmes trouvés."""
    problems = []
    for element, items in plan.instances.items():
        for p in items:
            if element != "roseaux" and element != "ronce" and terrain.height(p.x, p.y) < 0.3:
                problems.append(f"{element} dans l'eau en ({p.x:.0f}, {p.y:.0f})")
            if element not in ("ronce",) and layout.distance_to_paths(p.x, p.y) < 1.0:
                problems.append(f"{element} sur un chemin en ({p.x:.0f}, {p.y:.0f})")
            if layout.zone_at(p.x, p.y) is None:
                problems.append(f"{element} hors du monde en ({p.x:.0f}, {p.y:.0f})")
    return problems


def draw(plan, catalog):
    from PIL import Image, ImageDraw
    image = Image.open(HERE / "output" / "preview.png").convert("RGB")
    draw_ = ImageDraw.Draw(image)
    for element, items in plan.instances.items():
        color = tuple(int(c * 255) for c in catalog[element].parts[-1].color)
        for p in items[::3] if len(items) > 20_000 else items:  # allège l'image pour l'herbe
            draw_.point((p.x / SCALE, p.y / SCALE), fill=color)
    path = HERE / "output" / "scatter_preview.png"
    image.save(path)
    return path


def main():
    start = time.perf_counter()
    layout, terrain, catalog, plan = build()
    print(f"plan calculé en {time.perf_counter() - start:.1f} s\n")
    report(plan)

    problems = check(plan, layout, terrain)
    print(f"\nvérifications : {len(problems)} problème(s)")
    for problem in problems[:10]:
        print("  -", problem)

    print(f"aperçu : {draw(plan, catalog)}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
