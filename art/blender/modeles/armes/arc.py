"""Arc : l'arme silencieuse de la chasse, fabriquée à l'Abri.

Références : art/concepts/ambiances/zones-1-3-5-lieux.jpeg (L'Abri) et
zone-4-site-des-portes-lieux.jpeg (Le Lac gelé) : arc en bois brun d'une seule pièce, bouts
légèrement recourbés, corde fine, poignée bandée de tissu, ligatures aux bouts.
Ajouts : appui-flèche en cuir, fil et nœud du bois (lowpoly/wood.py).

Debout selon z, ventre de l'arc vers +x, corde à x = 0. Hauteur 1,3 m.
"""
import math

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shapes import box
from lowpoly.tool_parts import TOOL_COLORS, cloth_band, material, shade_wood
from lowpoly.wood import grain_lines

ICON = True
VIEW = (0.0, -1.0, 0.15)
ICON_VIEW = (0.35, -1.0, 0.15)

HALF = 0.65          # demi-hauteur de l'arc
BELLY = 0.17         # écart du milieu de l'arc à la corde
RECURVE = 0.03       # les bouts reviennent vers l'avant
SIDES = 6
FLATTEN = 1.7        # branche plus large (y) qu'épaisse (x)
STRING = (0.82, 0.76, 0.62)
LEATHER = (0.36, 0.22, 0.14)


def center_x(z):
    """Courbe de l'arc : ventre au milieu, bouts recourbés vers l'avant."""
    t = (z - HALF) / HALF
    return BELLY * math.cos(t * math.pi / 2) ** 0.8 + RECURVE * abs(t) ** 6


def radius_at(z):
    """Épais à la poignée, fin aux bouts (épaisseur selon x ; la largeur est × FLATTEN)."""
    t = abs(z - HALF) / HALF
    return 0.016 - 0.0095 * t ** 0.9


def _limbs():
    """L'arc en bois d'une pièce, tons en veines, traits du fil."""
    zs = [2 * HALF * i / 20 for i in range(21)]
    bow = tube("arc", [(center_x(z), 0.0, z) for z in zs], [radius_at(z) for z in zs],
               material("bois"), sides=SIDES, flatten=FLATTEN)
    lines = grain_lines("arc", center_x, radius_at, 0.12, 2 * HALF - 0.12, TOOL_COLORS["bois_fil"],
                        SIDES, FLATTEN, count=10, seed=4)
    return [shade_wood(bow, seed=4), *lines]


def _grip():
    """Poignée bandée, appui-flèche en cuir au-dessus."""
    band = cloth_band("poignee", center_x, radius_at, HALF - 0.06, HALF + 0.05, seed=2,
                      flatten=FLATTEN)
    rest = box("appui", (0.012, 0.03, 0.018), flat_material("arc_cuir", LEATHER),
               location=(center_x(HALF + 0.06) - 0.012, 0, HALF + 0.065))
    return band + [rest]


def _string():
    """Corde tendue d'un bout à l'autre, avec une ligature à chaque bout."""
    ends = [0.02, 2 * HALF - 0.02]
    rope = flat_material("arc_corde", STRING)
    parts = [tube("corde", [(center_x(z) - radius_at(z), 0.0, z) for z in ends], [0.0018] * 2,
                  rope, sides=4)]
    cord = material("corde")
    for index, z in enumerate(ends):
        lashing = [(center_x(z + dz), 0.0, z + dz) for dz in (-0.012, 0.012)]
        parts.append(tube(f"ligature_{index}", lashing, [radius_at(z) * 1.25] * 2, cord, sides=SIDES,
                          flatten=FLATTEN))
    return parts


def build():
    return _limbs() + _grip() + _string()
