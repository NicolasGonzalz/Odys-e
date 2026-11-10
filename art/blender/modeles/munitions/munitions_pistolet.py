"""Munitions de pistolet : trois balles de revolver retrouvées dans les ruines (lowpoly/cartridge.py).
Rares : on en trouve quelques-unes à la fois, d'où un petit tas plutôt qu'une boîte pleine.

Douilles en laiton terni (patine), balles en plomb à pointe ronde, pour le revolver (armes/pistolet).
Tas de 3, environ 5 cm.
"""
from lowpoly.cartridge import CASE_RADIUS, heap, revolver_round

ICON = True
VIEW = (0.35, -1.0, 0.8)
OUTLINE_THICKNESS = 0.0008   # objet minuscule : contour fin, sinon la balle disparaît dans son trait

RIM = CASE_RADIUS * 1.13

# Deux balles côte à côte au sol, la troisième posée dans le creux entre elles.
POSES = [(0.0, 0.0, RIM, 0.0), (0.004, 0.0108, RIM, 14.0), (0.009, 0.0052, RIM + 0.0083, -9.0)]


def build():
    return heap(revolver_round, POSES)
