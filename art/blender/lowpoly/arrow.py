"""Flèche universelle d'Odyssée : la même munition pour l'arc et l'arbalète (un seul objet
d'inventaire, plus simple en coop). Courte et robuste (0,55 m), pour passer dans les deux.

Hampe en bois clair, pointe plate taillée dans une tôle récupérée, ligaturée en corde, trois
pennes en tissu, encoche sombre au talon. Construite le long de +x à partir de l'encoche.
"""
import math

from .materials import flat_material
from .organic import tube
from .outline import without_outline
from .profile import profile_solid
from .shapes import cylinder
from .tool_parts import TOOL_COLORS, material

LENGTH = 0.55
SHAFT_RADIUS = 0.0045
VANE = (0.085, 0.022)   # longueur et hauteur d'une penne (m)

# Pointe vue de dessus (x depuis le bout de la hampe, z) : losange plat, épaulé.
HEAD = [(0.0, -0.0045), (0.016, -0.013), (0.046, 0.0), (0.016, 0.013), (0.0, 0.0045)]


def _shaft(origin):
    x, y, z = origin
    wood = flat_material("fleche_hampe", TOOL_COLORS["bois_bout"])
    shaft = tube("fleche_hampe", [(x + 0.006, y, z), (x + LENGTH - 0.04, y, z)],
                 [SHAFT_RADIUS] * 2, wood, sides=6)
    nock = cylinder("fleche_encoche", SHAFT_RADIUS * 1.15, 0.012, material("bois_sombre"),
                    vertices=6, location=(x + 0.006, y, z), rotation_deg=(0, 90, 0))
    return [shaft, nock]


def _head(origin):
    """Pointe en tôle (fine, chanfrein léger) et sa ligature en corde."""
    x, y, z = origin
    tip_x = x + LENGTH - 0.046
    outline = [(tip_x + px, z + pz) for px, pz in HEAD]
    head = profile_solid("fleche_pointe", outline, lambda _: 0.0022, [material("metal_clair")],
                         lambda _: 0, chamfer=0.0006)
    head.location.y = y
    tie = tube("fleche_ligature", [(tip_x - 0.018, y, z), (tip_x + 0.004, y, z)],
               [SHAFT_RADIUS * 1.35] * 2, material("corde"), sides=6)
    return [head, tie]


def _vanes(origin, roll):
    """Trois pennes en tissu à 120°, près du talon. `roll` tourne l'ensemble autour de la hampe.
    Pas de contour noir : sur une pièce de 1 mm il ferait un gros trait."""
    x, y, z = origin
    length, height = VANE
    base = SHAFT_RADIUS * 0.6
    # Angles ouverts : un angle trop aigu serait pris pour un aller-retour par clean_outline.
    outline = [(0.015, base), (0.015 + length, base), (0.005 + length, base + height * 0.45),
               (0.04, base + height), (0.015, base + height)]
    vanes = []
    for index in range(3):
        vane = profile_solid(f"fleche_penne_{index}", outline, lambda _: 0.0012,
                             [material("tissu")], lambda _: 0, chamfer=0.0003)
        vane.location = (x, y, z)
        vane.rotation_euler = (math.radians(roll + 120 * index), 0.0, 0.0)
        vanes.append(without_outline(vane))
    return vanes


def arrow(origin=(0.0, 0.0, 0.0), roll=0.0):
    """Toutes les pièces d'une flèche, de l'encoche (`origin`) vers +x."""
    return _shaft(origin) + _head(origin) + _vanes(origin, roll)
