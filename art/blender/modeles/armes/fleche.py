"""Flèche universelle : la munition de l'arc ET de l'arbalète (lowpoly/arrow.py).

Pas de concept dédié : style des carreaux et des flèches des ambiances (Abri, Lac gelé, Terres
noyées), pointe en tôle récupérée. Couchée, pointe vers +x, longueur 0,55 m.
"""
from lowpoly.arrow import arrow

ICON = True
VIEW = (0.2, -1.0, 0.5)
ICON_VIEW = (0.0, -1.0, 0.8)


def build():
    return arrow((0.0, 0.0, 0.013))
