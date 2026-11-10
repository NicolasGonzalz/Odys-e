"""Ce que la génération décide de poser : un élément, à une position, avec une orientation."""
from dataclasses import dataclass, field


@dataclass
class Placement:
    """Un élément posé dans le monde. Positions en mètres (x est, y sud), z = hauteur du sol."""
    element: str
    x: float
    y: float
    z: float
    yaw_deg: float = 0.0
    # Facteur par axe ; pour les bâtiments : largeur, profondeur, hauteur en mètres.
    scale: tuple = (1.0, 1.0, 1.0)


@dataclass
class LabeledActor:
    """Un élément important posé seul, avec un nom lisible (lieux, boss, artefacts, cocons)."""
    label: str
    folder: str
    placement: Placement
    tags: list = field(default_factory=list)


@dataclass
class WorldPlan:
    """Tout ce que la génération a décidé, avant de le poser dans Unreal."""
    instances: dict = field(default_factory=dict)  # élément -> [Placement] (végétation, immeubles)
    labeled: list = field(default_factory=list)    # [LabeledActor]
    spawn: tuple = (0.0, 0.0, 0.0)                 # point d'apparition des joueurs

    def add(self, placement):
        self.instances.setdefault(placement.element, []).append(placement)

    def count(self):
        return sum(len(items) for items in self.instances.values()) + len(self.labeled)
