"""Squelette d'échelle (1,75 m) : un repère pour juger la taille des objets en main.

Ce n'est pas un objet du jeu (EXPORT = False : pas de .fbx). Debout, tourné vers -y ; bras gauche
le long du corps, main droite tendue devant à hauteur de taille, comme pour tenir un outil.
HAND donne la position de la main droite (y poser le manche d'un outil). Proportions moyennes
d'un adulte : à adapter si les survivants ont des tailles différentes.
"""
from mathutils import Vector

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shapes import blob, box, cylinder
from lowpoly.outline import without_outline

EXPORT = False
VIEW = (0.6, -1.0, 0.25)

BONE = (0.86, 0.83, 0.74)
SOCKET = (0.12, 0.11, 0.1)
HEIGHT = 1.75

# Articulations (m). Le personnage regarde vers -y : sa droite est en -x.
J = {
    "cou": (0, 0, 1.5), "epaule_d": (-0.19, 0, 1.44), "epaule_g": (0.19, 0, 1.44),
    "coude_d": (-0.23, -0.06, 1.16), "poignet_d": (-0.24, -0.31, 1.05),
    "coude_g": (0.24, 0.01, 1.13), "poignet_g": (0.26, 0.0, 0.86),
    "bassin": (0, 0, 0.98), "hanche_d": (-0.1, 0, 0.93), "hanche_g": (0.1, 0, 0.93),
    "genou_d": (-0.11, -0.02, 0.5), "genou_g": (0.11, -0.02, 0.5),
    "cheville_d": (-0.11, 0.01, 0.08), "cheville_g": (0.11, 0.01, 0.08),
}
HAND = (-0.245, -0.37, 1.03)   # milieu de la main droite fermée

LIMBS = [("cou", "bassin", 0.022), ("epaule_d", "coude_d", 0.022), ("coude_d", "poignet_d", 0.018),
         ("epaule_g", "coude_g", 0.022), ("coude_g", "poignet_g", 0.018),
         ("hanche_d", "genou_d", 0.027), ("genou_d", "cheville_d", 0.022),
         ("hanche_g", "genou_g", 0.027), ("genou_g", "cheville_g", 0.022),
         ("epaule_d", "epaule_g", 0.016)]


def _bones(bone):
    parts = [tube(f"os_{a}_{b}", [J[a], J[b]], [r, r * 0.85], bone, sides=6) for a, b, r in LIMBS]
    parts += [blob(f"articulation_{name}", 0.032 if "genou" in name or "hanche" in name else 0.026,
                   bone, location=J[name]) for name in J if name not in ("cou", "bassin")]
    return parts


def _skull(bone):
    """Crâne, mâchoire, orbites sombres (tournées vers -y)."""
    skull = blob("crane", 0.1, bone, subdivisions=2, scale=(0.82, 0.95, 1.0), location=(0, 0, 1.64))
    jaw = box("machoire", (0.1, 0.08, 0.05), bone, location=(0, -0.035, 1.555))
    dark = flat_material("squelette_orbite", SOCKET)
    eyes = [without_outline(cylinder(f"orbite_{s}", 0.019, 0.01, dark, vertices=6,
                                     location=(s * 0.035, -0.088, 1.65), rotation_deg=(90, 0, 0)))
            for s in (-1, 1)]
    return [skull, jaw, *eyes]


def _torso(bone):
    """Cage thoracique (côtes en anneaux), bassin, sternum."""
    parts = []
    for index in range(5):
        z = 1.38 - index * 0.055
        half_w, half_d = 0.13 - index * 0.004, 0.085
        ring = [(half_w * c, -half_d * s - 0.01, z - 0.02 * abs(s))
                for c, s in ((1, 0), (0.7, 0.7), (0, 1), (-0.7, 0.7), (-1, 0), (-0.7, -0.7),
                             (0, -1), (0.7, -0.7), (1, 0))]
        parts.append(tube(f"cote_{index}", ring, [0.011] * len(ring), bone, sides=4))
    parts.append(box("sternum", (0.03, 0.02, 0.2), bone, location=(0, -0.095, 1.3)))
    parts.append(blob("bassin_os", 0.1, bone, scale=(1.5, 0.8, 0.7), location=(0, 0, 0.98)))
    return parts


def _extremities(bone):
    """Mains (la droite fermée sur un manche) et pieds."""
    parts = [blob("main_d", 0.045, bone, scale=(0.8, 1.0, 1.1), location=HAND),
             blob("main_g", 0.045, bone, scale=(0.6, 0.8, 1.3), location=(0.27, 0.0, 0.8))]
    for side in (-1, 1):
        heel = Vector(J["cheville_d" if side < 0 else "cheville_g"])
        parts.append(box(f"pied_{side}", (0.08, 0.24, 0.05), bone,
                         location=(heel.x, heel.y - 0.07, 0.025)))
    return parts


def build():
    bone = flat_material("squelette_os", BONE)
    return _bones(bone) + _skull(bone) + _torso(bone) + _extremities(bone)
