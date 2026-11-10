"""Munitions de fusil à pompe : trois cartouches retrouvées dans les ruines (lowpoly/cartridge.py).
Rares et précieuses : peu de coups, mais qui comptent.

Culots en laiton terni, tubes en plastique rouge pâli par le soleil, sertissage en étoile, pour le
fusil à pompe (armes/fusil_pompe). Tas de 3, environ 7 cm.
"""
from lowpoly.cartridge import SHELL_RADIUS, heap, shotgun_shell

ICON = True
VIEW = (0.35, -1.0, 0.8)
OUTLINE_THICKNESS = 0.0013   # petit objet : contour plus fin que sur les outils

RIM = SHELL_RADIUS * 1.16

# Deux cartouches côte à côte au sol, la troisième posée dans le creux entre elles.
POSES = [(0.0, 0.0, RIM, 0.0), (0.008, 0.0212, RIM, -12.0), (0.014, 0.0104, RIM + 0.0168, 7.0)]


def build():
    return heap(shotgun_shell, POSES)
