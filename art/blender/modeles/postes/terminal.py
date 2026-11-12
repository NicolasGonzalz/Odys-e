"""Terminal : une console d'avant, encore sous tension. On y lit des indices (journaux des
opérateurs), et il peut servir d'épreuve dans un niveau (rétablir le courant, entrer un code).

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/terminal.png).
Meuble en tôle grise rouillée : deux écrans (l'un éteint, l'autre affichant du texte vert), un
pupitre incliné avec clavier et pavé de touches rouges, un lecteur de carte sur le côté, une
trappe sur le bas, grilles d'aération, voyant allumé, plaque du constructeur effacée, câbles qui
sortent du dos et rampent au sol. Les écrans et le voyant émettent de la lumière. 0,8 m de large, 1,6 m de haut.
"""
import math

from lowpoly.carpentry import stain
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.organic import tube
from lowpoly.shapes import bevel, box
from lowpoly.tool_parts import material

ICON = True
VIEW = (-0.6, -1.0, 0.65)   # de trois quarts et de haut : on voit le pupitre

WIDTH = 0.8
GREY = (0.72, 0.71, 0.68)
GREY_DARK = (0.52, 0.52, 0.51)
SCREEN_OFF = (0.13, 0.17, 0.15)
SCREEN_ON = (0.08, 0.20, 0.10)
TEXT = (0.45, 1.0, 0.45)
KEY = (0.78, 0.78, 0.76)
KEY_RED = (0.78, 0.22, 0.16)
RUST = (0.66, 0.36, 0.20)

# Profil du meuble vu de côté (profondeur, hauteur) : l'avant est vers la profondeur négative.
PROFILE = [(-0.24, 0.0), (0.3, 0.0), (0.3, 1.6), (-0.12, 1.6), (-0.16, 1.08), (-0.5, 0.94),
           (-0.5, 0.84), (-0.24, 0.8)]
DESK = ((-0.16, 1.08), (-0.5, 0.94))    # arête haute et arête avant du pupitre
DESK_TILT = math.degrees(math.atan2(DESK[0][1] - DESK[1][1], DESK[0][0] - DESK[1][0]))


def _cabinet():
    """Le meuble : profil épaissi sur la largeur, puis tourné pour que l'avant regarde vers -y."""
    body = profile_solid("terminal", PROFILE, lambda _: WIDTH, [flat_material("terminal_gris", GREY)],
                         lambda _: 0, cuts_x=(-0.3, -0.16, 0.0, 0.15), chamfer=0.03, segments=2)
    two_tone(body, GREY, GREY_DARK, "terminal_gris", dark_side=(0.6, 0.4, -0.3), threshold=0.1)
    return posed([body], (0, 0, 90), (0, 0, 0))




def _screens():
    """Deux écrans dans leur cadre : l'un éteint, l'autre plein de lignes de texte vert."""
    face_y = -0.155
    parts = []
    for i, (x, on) in enumerate(((-0.19, False), (0.19, True))):
        frame = bevel(box(f"ecran_cadre_{i}", (0.36, 0.03, 0.34), material("metal_sombre"),
                          location=(x, face_y - 0.012, 1.34)), 0.008)
        screen = box(f"ecran_{i}", (0.3, 0.01, 0.28),
                     flat_material(f"ecran_{on}", SCREEN_ON if on else SCREEN_OFF,
                                   emission=1.5 if on else 0.0),
                     location=(x, face_y - 0.03, 1.34))
        parts += [frame, screen]
        if on:
            parts += _text_lines(x, face_y - 0.036)
    return parts


def _text_lines(x, y):
    """Lignes de texte vert, de longueurs inégales."""
    glow = flat_material("ecran_texte", TEXT, emission=4.0)
    lengths = (0.24, 0.17, 0.21, 0.09, 0.2, 0.14, 0.18)
    return [box(f"texte_{i}", (length, 0.002, 0.014), glow,
                location=(x - 0.125 + length / 2, y, 1.45 - i * 0.034))
            for i, length in enumerate(lengths)]


def _desk_items():
    """Clavier et pavé de touches rouges posés sur le pupitre incliné, lecteur de carte."""
    (back_y, back_z), (front_y, front_z) = DESK
    tilt = 90 - DESK_TILT  # incliné comme le pupitre
    parts = []
    for row in range(4):  # clavier : quatre rangées de touches
        t = (row + 0.6) / 4.6
        y = back_y + (front_y - back_y) * t
        z = back_z + (front_z - back_z) * t + 0.012
        parts.append(box(f"clavier_rang_{row}", (0.44, 0.055, 0.02), flat_material("touche", KEY),
                         location=(-0.1, y, z), rotation_deg=(tilt - 90, 0, 0)))
    for i in range(3):
        for j in range(2):
            t = (j + 1.2) / 3.4
            parts.append(box(f"touche_rouge_{i}_{j}", (0.045, 0.045, 0.02),
                             flat_material("touche_rouge", KEY_RED),
                             location=(0.2 + i * 0.055, back_y + (front_y - back_y) * t,
                                       back_z + (front_z - back_z) * t + 0.012),
                             rotation_deg=(tilt - 90, 0, 0)))
    parts.append(bevel(box("lecteur", (0.08, 0.1, 0.16), material("metal_sombre"),
                           location=(0.33, -0.3, 1.0)), 0.01))
    return parts


def _lower_panel():
    """Trappe d'accès sur le bas de la façade, et ses vis."""
    hatch = box("trappe", (0.4, 0.012, 0.45), flat_material("terminal_trappe", GREY_DARK),
                location=(0.0, -0.246, 0.36))
    return [bevel(hatch, 0.01)]


def _rust():
    """Plaques de rouille sur la façade, les côtés et le dessus."""
    parts = []
    spots = (((-0.32, 1.52), 0.05), ((0.3, 0.6), 0.06), ((-0.25, 0.2), 0.05), ((0.1, 0.72), 0.04))
    for i, ((x, z), r) in enumerate(spots):
        patch = stain(f"rouille_face_{i}", (0.0, 0.0, 0.0), r, RUST, seed=i)
        parts += posed([patch], (90, 0, 0), (x, -0.246 if z < 0.8 else -0.126, z))
    for i, (side, y, z) in enumerate(((-1, 0.0, 1.2), (1, 0.15, 0.5))):
        patch = stain(f"rouille_cote_{i}", (0.0, 0.0, 0.0), 0.07, RUST, seed=10 + i)
        parts += posed([patch], (90, 0, 90 * side), (side * (WIDTH / 2 + 0.0015), y, z))
    parts.append(stain("rouille_dessus", (0.1, 0.1, 1.6015), 0.08, RUST, seed=20))
    return parts


def _vents():
    """Grilles d'aération : sur les deux côtés, et sur le bas de la façade."""
    dark = flat_material("terminal_grille", (0.20, 0.20, 0.21))
    parts = []
    for side in (-1, 1):
        for k in range(5):
            parts.append(box(f"aeration_{side}_{k}", (0.004, 0.22, 0.018), dark,
                             location=(side * (WIDTH / 2 + 0.002), 0.05, 1.3 - k * 0.04)))
    for k in range(4):
        parts.append(box(f"aeration_bas_{k}", (0.3, 0.004, 0.016), dark,
                         location=(0.0, -0.254, 0.1 + k * 0.035)))
    return parts


def _details():
    """Voyant allumé, plaque du constructeur effacée, socle, câbles qui sortent du dos."""
    parts = [box("voyant", (0.022, 0.012, 0.022),
                 flat_material("terminal_voyant", (0.4, 1.0, 0.4), emission=4.0),
                 location=(0.33, -0.205, 0.74)),
             box("plaque_constructeur", (0.16, 0.004, 0.05),
                 flat_material("terminal_plaque", (0.80, 0.78, 0.70)),
                 location=(-0.22, -0.248, 0.72)),
             bevel(box("socle", (WIDTH + 0.04, 0.6, 0.04), material("metal_sombre"),
                       location=(0.0, 0.03, 0.02)), 0.01)]
    cable = flat_material("terminal_cable", (0.12, 0.12, 0.13))
    for i, (x, color) in enumerate(((-0.2, cable), (0.05, cable),
                                    (0.22, flat_material("cable_rouge", (0.55, 0.16, 0.12))))):
        path = [(x, 0.3, 0.5 - i * 0.08), (x + 0.05, 0.42, 0.3), (x + 0.12, 0.55, 0.03),
                (x + 0.3, 0.75, 0.02)]
        parts.append(tube(f"cable_{i}", path, [0.016] * len(path), color, sides=6))
    return parts


def build():
    return (_cabinet() + _screens() + _desk_items() + _lower_panel() + _vents() + _details()
            + _rust())
