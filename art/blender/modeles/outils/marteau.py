"""Marteau (outil) : pour construire en mode bac à sable (maisons, abris), plus tard.

Marteau de charpentier à panne fendue (lowpoly/hammer.py), fabriqué à l'établi. Debout, face de
frappe vers +x. Longueur 0,36 m.
"""
from lowpoly.hammer import hammer

ICON = True
VIEW = (0.2, -1.0, 0.2)
PREVIEW_ROTATION_DEG = (0, 22, 0)


def build():
    return hammer(seed=3)
