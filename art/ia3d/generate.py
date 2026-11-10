#!/usr/bin/env python3
"""Génère des modèles 3D par IA à partir des planches, puis les prépare pour Unreal.

Étapes pour chaque nom :
  1. découpe de l'objet dans sa planche        (crop.py)      -> art/ia3d/crops/<nom>.png
  2. génération par Meshy                      (meshy.py)     -> art/ia3d/generated/<nom>.glb
  3. mise à l'échelle, pivot, export, aperçu   (Blender)      -> art/exports/<catégorie>/<nom>.fbx

Usage :
    export MESHY_API_KEY=...                       # une fois par terminal
    python3 art/ia3d/generate.py hache loup_gaia   # consomme des crédits Meshy
    python3 art/ia3d/generate.py --skip-meshy hache  # refait seulement l'étape 3 (gratuit)
"""
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))

from crop import crop, load_models  # noqa: E402
from meshy import MeshyError, create_task, download, wait_for  # noqa: E402


def generate(name, spec):
    image = crop(name, spec)
    print(f"[{name}] envoi à Meshy…")
    task = create_task(image, spec.get("pose", ""))
    url = wait_for(task)
    glb = download(url, HERE / "generated" / f"{name}.glb")
    print(f"[{name}] modèle reçu : {glb.relative_to(ROOT)}")


def prepare_in_blender(names):
    """Lance Blender sans fenêtre pour mettre les modèles à l'échelle et les exporter."""
    script = ROOT / "art" / "blender" / "import_ia.py"
    subprocess.run(["blender", "-b", "--python", str(script), "--", *names], check=True)


def main():
    args = sys.argv[1:]
    skip_meshy = "--skip-meshy" in args
    names = [a for a in args if not a.startswith("--")]
    models = load_models()
    unknown = [n for n in names if n not in models]
    if not names or unknown:
        raise SystemExit(f"Noms disponibles : {', '.join(models)}")

    if not skip_meshy:
        for name in names:
            try:
                generate(name, models[name])
            except MeshyError as error:
                raise SystemExit(f"[{name}] échec : {error}") from error
    prepare_in_blender(names)


if __name__ == "__main__":
    main()
