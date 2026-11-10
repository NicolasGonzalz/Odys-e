"""Jumelles : repérer les Gardiens et les ressources de loin.

Référence : art/concepts/ambiances/zone-4-site-des-portes-lieux.jpeg (Col du plateau) :
jumelles militaires trapues, corps vert olive gainé de caoutchouc sombre, grosses lentilles vertes.
Ajouts : charnière centrale avec molette de mise au point, œilletons, anneaux de sangle.

Couchées, oculaires vers -x, objectifs vers +x. Longueur 0,15 m.
"""
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.shapes import box, cylinder

ICON = True
VIEW = (-0.6, -1.0, 0.7)   # on voit les oculaires et le dessus

OLIVE = (0.48, 0.51, 0.41)
OLIVE_DARK = (0.32, 0.35, 0.28)
RUBBER = (0.20, 0.21, 0.20)
LENS = (0.30, 0.62, 0.40)
SPACING = 0.034   # demi-écart entre les deux tubes
AXIS_Z = 0.032

# Profil d'un tube (x, rayon) : oculaire fin, corps, objectif plus large.
BARREL = [(0.0, 0.014), (0.022, 0.015), (0.03, 0.022), (0.1, 0.024), (0.11, 0.029), (0.15, 0.03)]


def _barrel(side):
    """Un tube : corps olive, œilleton en caoutchouc, lentille verte au bout de l'objectif."""
    y = side * SPACING
    body = tube(f"tube_{side}", [(x, y, AXIS_Z) for x, _ in BARREL], [r for _, r in BARREL],
                flat_material("jumelles_olive", OLIVE), sides=8)
    two_tone(body, OLIVE, OLIVE_DARK, f"jumelles_{side}", dark_side=(0, 0.2, -1), seed=side + 2)
    rubber = flat_material("jumelles_caoutchouc", RUBBER)
    eyecup = cylinder(f"oeilleton_{side}", 0.016, 0.018, rubber, vertices=8,
                      location=(-0.006, y, AXIS_Z), rotation_deg=(0, 90, 0))
    grip_band = cylinder(f"gaine_{side}", 0.0255, 0.05, rubber, vertices=8,
                         location=(0.065, y, AXIS_Z), rotation_deg=(0, 90, 0))
    lens = cylinder(f"lentille_{side}", 0.024, 0.003, flat_material("jumelles_verre", LENS,
                    emission=0.4), vertices=8, location=(0.15, y, AXIS_Z), rotation_deg=(0, 90, 0))
    eyepiece = cylinder(f"oculaire_{side}", 0.009, 0.003, flat_material("jumelles_noir", (0.05,) * 3),
                        vertices=8, location=(-0.0155, y, AXIS_Z), rotation_deg=(0, 90, 0))
    return [body, eyecup, grip_band, lens, eyepiece]


def _bridge():
    """Charnière entre les tubes, molette de mise au point, anneaux de sangle."""
    dark = flat_material("jumelles_olive_sombre", OLIVE_DARK)
    parts = [box("charniere", (0.07, 2 * SPACING - 0.03, 0.016), dark,
                 location=(0.055, 0, AXIS_Z + 0.006))]
    parts.append(cylinder("molette", 0.011, 0.022, flat_material("jumelles_caoutchouc", RUBBER),
                          vertices=10, location=(0.03, 0, AXIS_Z + 0.016), rotation_deg=(90, 0, 0)))
    for side in (-1, 1):
        parts.append(cylinder(f"anneau_{side}", 0.006, 0.004, dark, vertices=6,
                              location=(0.01, side * (SPACING + 0.02), AXIS_Z),
                              rotation_deg=(90, 0, 0)))
    return parts


def build():
    return _barrel(-1) + _barrel(1) + _bridge()
