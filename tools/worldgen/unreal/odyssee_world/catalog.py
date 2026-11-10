"""Catalogue des éléments (elements.json) : vrai modèle ou forme provisoire."""
import json
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Part:
    """Une forme simple de la version provisoire d'un élément."""
    shape: str      # cube, sphere, cylinder, cone
    color: tuple    # rouge, vert, bleu entre 0 et 1
    size: tuple     # mètres
    offset: tuple   # centre de la forme depuis le pied de l'élément, en mètres


@dataclass
class Element:
    name: str
    mesh: str           # chemin du vrai modèle Unreal ; vide = forme provisoire
    parts: list
    collision: bool
    cull_m: float
    scale_range: tuple
    emissive: float

    @property
    def has_real_mesh(self):
        return bool(self.mesh)


def _element(name, data):
    parts = [Part(p["shape"], tuple(p["color"]), tuple(p["size"]), tuple(p["offset"]))
             for p in data["parts"]]
    return Element(name=name, mesh=data.get("mesh", ""), parts=parts,
                   collision=data.get("collision", True), cull_m=data.get("cull_m", 0),
                   scale_range=tuple(data.get("scale", [1, 1])),
                   emissive=data.get("emissive", 0))


def load_catalog(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    return {name: _element(name, value) for name, value in data.items() if not name.startswith("_")}


def load_rules(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    return {region: rules for region, rules in data.items() if not region.startswith("_")}
