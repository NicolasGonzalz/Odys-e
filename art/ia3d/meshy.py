"""Client minimal de l'API Meshy « image → 3D » (bibliothèque standard uniquement).

La clé est lue dans la variable d'environnement MESHY_API_KEY : jamais dans le code ni le dépôt.
Documentation de l'API : https://docs.meshy.ai
"""
import base64
import json
import os
import time
import urllib.error
import urllib.request
from pathlib import Path

API = "https://api.meshy.ai/openapi/v1/image-to-3d"
POLL_SECONDS = 10
TIMEOUT_SECONDS = 15 * 60


class MeshyError(RuntimeError):
    pass


def _api_key():
    key = os.environ.get("MESHY_API_KEY", "").strip()
    if not key:
        raise MeshyError("MESHY_API_KEY absente : `export MESHY_API_KEY=...` dans le terminal")
    return key


def _request(url, payload=None):
    """Appel HTTP JSON ; les erreurs de Meshy sont remontées avec leur message."""
    data = json.dumps(payload).encode() if payload is not None else None
    request = urllib.request.Request(url, data=data, method="POST" if data else "GET", headers={
        "Authorization": f"Bearer {_api_key()}",
        "Content-Type": "application/json",
    })
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            return json.loads(response.read())
    except urllib.error.HTTPError as error:
        detail = error.read().decode(errors="replace")[:300]
        hints = {401: "clé invalide", 402: "crédits Meshy insuffisants", 429: "trop de requêtes"}
        hint = hints.get(error.code, "")
        raise MeshyError(f"Meshy HTTP {error.code} ({hint}) : {detail}") from error


def _data_uri(image_path):
    encoded = base64.b64encode(Path(image_path).read_bytes()).decode()
    return f"data:image/png;base64,{encoded}"


def create_task(image_path, pose=""):
    """Lance la génération ; renvoie l'identifiant de la tâche."""
    payload = {
        "image_url": _data_uri(image_path),
        "ai_model": "latest",
        "model_type": "lowpoly",       # maillage propre et léger, adapté au jeu
        "topology": "triangle",
        "should_texture": True,
        "enable_pbr": False,           # couleurs unies, comme les planches
        "remove_lighting": True,       # pas d'ombres « peintes » dans la texture
        "symmetry_mode": "auto",
        "pose_mode": pose,             # "a-pose" pour les personnages
        "target_formats": ["glb"],
    }
    return _request(API, payload)["result"]


def wait_for(task_id, on_progress=print):
    """Attend la fin de la tâche ; renvoie l'adresse du fichier .glb."""
    started = time.monotonic()
    while time.monotonic() - started < TIMEOUT_SECONDS:
        task = _request(f"{API}/{task_id}")
        status = task.get("status")
        if status == "SUCCEEDED":
            return task["model_urls"]["glb"]
        if status in ("FAILED", "CANCELED", "EXPIRED"):
            raise MeshyError(f"tâche {status} : {task.get('task_error', {}).get('message', '')}")
        on_progress(f"  {status} {task.get('progress', 0)} %")
        time.sleep(POLL_SECONDS)
    raise MeshyError("délai dépassé (15 min)")


def download(url, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=120) as response:
        path.write_bytes(response.read())
    return path
