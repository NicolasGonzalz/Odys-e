"""Pied de biche : ouvrir les caisses et les portes coincées, et se défendre en ville.

Références : art/concepts/ambiances/zone-2-ville-engloutie.jpg et zone-4-site-des-portes-lieux.jpeg
(Les Hangars) : barre à six pans peinte en rouge, écaillée jusqu'au métal et rouillée, col de
cygne au bout fendu en griffe, poignée bandée de tissu.

Debout selon z, griffe en haut vers -x. Hauteur 0,76 m (un vrai pied de biche de démolition, tenu à deux mains si besoin).
"""
import math

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import wedge
from lowpoly.tool_parts import grip, material

ICON = True
VIEW = (0.2, -1.0, 0.2)
PREVIEW_ROTATION_DEG = (0, 22, 0)   # incliné comme en main sur le concept

PAINT = (0.56, 0.22, 0.15)
PAINT_DARK = (0.38, 0.15, 0.11)
RADIUS = 0.013
SHAFT = 0.66        # longueur de la partie droite
HOOK = 0.052        # rayon du col de cygne
CURL = 200          # le col s'enroule un peu au-delà du demi-tour (degrés)


def _hook_point(degrees):
    angle = math.radians(degrees)
    return (-HOOK + HOOK * math.cos(angle), 0.0, SHAFT + HOOK * math.sin(angle))


def _path():
    """Ligne de la barre : pied biseauté, partie droite, col de cygne enroulé."""
    points = [(0.018, 0, -0.022), (0.006, 0, 0.0)]
    points += [(0, 0, z) for z in (0.06, 0.2, 0.34, 0.5, SHAFT)]
    return points + [_hook_point(CURL * step / 10) for step in range(1, 11)]


def _bar():
    """Barre à six pans, peinte, écaillée (métal) et rouillée."""
    points = _path()
    radii = [RADIUS * 0.45, RADIUS * 0.85] + [RADIUS] * (len(points) - 2)
    bar = tube("barre", points, radii, flat_material("pied_peinture", PAINT), sides=6)
    two_tone(bar, PAINT, PAINT_DARK, "pied_peinture", dark_side=(1.0, -0.3, 0.0), seed=2)
    blotches(bar, material("metal_sombre"), count=6, radius=0.03, seed=3)
    return blotches(bar, material("rouille"), count=5, radius=0.035, seed=4)


def _claw():
    """Griffe au bout du col : deux dents plates biseautées, séparées par une fente,
    dans le prolongement de la courbe. Métal mis à nu par l'usage."""
    angle = math.radians(CURL)
    tx, tz = -math.sin(angle), math.cos(angle)   # direction de la courbe à son bout
    tip = _hook_point(CURL)
    pitch = math.degrees(math.atan2(-tz, tx))
    worn = material("metal")
    teeth = []
    for side in (-1, 1):
        tooth = wedge(f"griffe_{side}", 0.05, (0.01, 0.022), (0.008, 0.0035), worn,
                      location=(tip[0] - tx * 0.004, side * 0.0062, tip[2] - tz * 0.004),
                      rotation_deg=(0, pitch, 0))
        teeth.append(two_tone(tooth, (0.68, 0.63, 0.57), (0.5, 0.48, 0.46), "pied_griffe",
                              dark_side=(1, -0.3, 0), seed=side + 3))
    return teeth


def build():
    hand = grip("poignee", lambda z: 0.0, lambda z: RADIUS, 0.03, 0.23, seed=5, flatten=1.0)
    return [_bar(), *_claw(), *hand]
