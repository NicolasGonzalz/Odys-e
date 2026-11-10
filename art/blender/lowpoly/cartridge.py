"""Munitions d'Odyssée : de vieilles cartouches retrouvées dans les ruines, rares et usées.

Chaque cartouche est construite debout (selon z, culot en bas), puis posée dans un tas (`heap`).
Laiton terni de patine, plomb mat, plastique délavé : de l'usure, pas de rouille franche (le
laiton ne rouille pas, il verdit et brunit).

  AMMO_COLORS   : laiton, patine, plomb, plastique rouge
  revolver_round: balle de revolver (douille en laiton, balle en plomb à pointe ronde)
  shotgun_shell : cartouche de fusil (culot en laiton, tube en plastique, sertissage en étoile)
  heap          : plusieurs cartouches couchées les unes contre les autres
"""
import math

from .finish import posed
from .materials import flat_material
from .organic import tube
from .outline import without_outline
from .shading import blotches, two_tone
from .shapes import cylinder

AMMO_COLORS = {
    "laiton": (0.80, 0.63, 0.30),
    "laiton_sombre": (0.57, 0.42, 0.19),
    "patine": (0.43, 0.41, 0.25),       # laiton verdi et bruni par les années
    "plomb": (0.56, 0.57, 0.59),
    "plomb_sombre": (0.40, 0.41, 0.44),
    "plastique": (0.70, 0.20, 0.15),
    "plastique_sombre": (0.49, 0.13, 0.10),
    "delave": (0.80, 0.50, 0.42),       # plastique pâli par le soleil
    "sertissage": (0.38, 0.10, 0.08),
}

SIDES = 10
RINGS = 6   # tronçons le long d'une douille : assez de faces pour que la patine fasse des taches


def _column(z0, z1):
    """Points d'un tube droit de z0 à z1, en RINGS tronçons."""
    return [(0, 0, z0 + (z1 - z0) * i / RINGS) for i in range(RINGS + 1)]


def ammo_material(name):
    return flat_material(f"munition_{name}", AMMO_COLORS[name])


def _brass(part, seed, patina=2, radius=0.004):
    """Laiton à deux tons, avec quelques plaques de patine."""
    two_tone(part, AMMO_COLORS["laiton"], AMMO_COLORS["laiton_sombre"], "munition_laiton",
             dark_side=(0.3, 0.4, -1.0), threshold=0.1, seed=seed)
    return blotches(part, ammo_material("patine"), count=patina, radius=radius, seed=seed)


def _rim(name, radius, z, seed):
    """Bourrelet du culot, un peu plus large que la douille."""
    rim = cylinder(f"{name}_bourrelet", radius, 0.0016, ammo_material("laiton"), vertices=SIDES,
                   location=(0, 0, z))
    return _brass(rim, seed, patina=0)


# --- Balle de revolver --------------------------------------------------------------------------

CASE_RADIUS = 0.0048
CASE_LENGTH = 0.029
BULLET = [(0.0, 0.0046), (0.004, 0.0045), (0.008, 0.0038), (0.011, 0.0025), (0.0128, 0.0006)]


def revolver_round(name, seed=0):
    """Douille en laiton (bourrelet au culot), balle en plomb à pointe ronde. Hauteur 4,2 cm."""
    case_points = _column(0.0, CASE_LENGTH)
    case = tube(f"{name}_douille", case_points, [CASE_RADIUS] * len(case_points),
                ammo_material("laiton"), sides=SIDES)
    bullet = tube(f"{name}_balle", [(0, 0, CASE_LENGTH + z) for z, _ in BULLET],
                  [r for _, r in BULLET], ammo_material("plomb"), sides=SIDES)
    two_tone(bullet, AMMO_COLORS["plomb"], AMMO_COLORS["plomb_sombre"], "munition_plomb",
             dark_side=(0.3, 0.4, -1.0), threshold=0.1, seed=seed)
    return [_brass(case, seed, patina=2, radius=0.0035), _rim(name, CASE_RADIUS * 1.13, 0.0008, seed),
            bullet]


# --- Cartouche de fusil -------------------------------------------------------------------------

SHELL_RADIUS = 0.0093
HEAD_LENGTH = 0.016
SHELL_LENGTH = 0.063


def shotgun_shell(name, seed=0):
    """Culot en laiton, tube en plastique rouge délavé, sertissage en étoile. Hauteur 6,3 cm."""
    head = tube(f"{name}_culot", [(0, 0, 0.0), (0, 0, HEAD_LENGTH)],
                [SHELL_RADIUS * 1.06] * 2, ammo_material("laiton"), sides=SIDES)
    body_points = _column(HEAD_LENGTH, SHELL_LENGTH)
    body = tube(f"{name}_tube", body_points, [SHELL_RADIUS] * len(body_points),
                ammo_material("plastique"), sides=SIDES)
    two_tone(body, AMMO_COLORS["plastique"], AMMO_COLORS["plastique_sombre"],
             "munition_plastique", dark_side=(0.3, 0.4, -1.0), threshold=0.1, seed=seed)
    blotches(body, ammo_material("delave"), count=2, radius=0.008, seed=seed + 3)
    head = _brass(head, seed, patina=1, radius=0.006)
    return [head, _rim(name, SHELL_RADIUS * 1.16, 0.0008, seed), body] + _star_crimp(name)


def _star_crimp(name):
    """Le dessus replié en étoile : un disque sombre et six plis qui partent du centre."""
    top = SHELL_LENGTH + 0.0004
    dark = ammo_material("sertissage")
    parts = [without_outline(cylinder(f"{name}_dessus", SHELL_RADIUS * 0.92, 0.0006, dark,
                                      vertices=SIDES, location=(0, 0, top)))]
    for fold in range(6):
        angle = math.tau * fold / 6
        tip = (math.cos(angle) * SHELL_RADIUS * 0.85, math.sin(angle) * SHELL_RADIUS * 0.85, top)
        parts.append(without_outline(tube(f"{name}_pli_{fold}", [(0, 0, top + 0.0003),
                                                                 (tip[0], tip[1], top + 0.0003)],
                                          [0.00045] * 2, ammo_material("plastique_sombre"),
                                          sides=3)))
    return parts


# --- Tas ----------------------------------------------------------------------------------------

def heap(build_round, poses):
    """Plusieurs cartouches couchées (selon x) en tas. `poses` : (décalage x, y, z, lacet en degrés)."""
    parts = []
    for index, (x, y, z, yaw) in enumerate(poses):
        cartridge = build_round(f"cartouche_{index}", seed=index + 1)
        parts += posed(cartridge, (0, 90, yaw), (x, y, z))
    return parts
