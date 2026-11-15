"""Peau (ressource) : obtenue en dépeçant un animal au couteau. Cuir, sacs, vêtements chauds.

Sans concept : peau de cerf tannée à la va-vite, étalée au sol, fourrure fauve vers le haut,
quatre pattes, cou et queue courte reconnaissables ; bord ondulé et taches plus sombres sur le dos.
~70 cm. Choix de design à valider.
"""
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.profile import profile_solid
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import roughen

ICON = True
VIEW = (0.2, -0.9, 1.3)

FUR = (0.64, 0.45, 0.28)         # fauve
FUR_DARK = (0.45, 0.30, 0.18)
BACK = (0.36, 0.24, 0.15)        # taches sombres du dos
LEATHER = (0.86, 0.76, 0.60)     # bord du cuir, côté chair

# Moitié haute du contour (x, z), du cou (x > 0) à la queue ; la moitié basse en est le miroir,
# légèrement déformé pour ne pas faire « découpé au gabarit ».
HALF = [(0.34, 0.035), (0.27, 0.07), (0.25, 0.16), (0.2, 0.2), (0.16, 0.18), (0.15, 0.1),
        (0.05, 0.125), (-0.08, 0.12), (-0.15, 0.1), (-0.18, 0.19), (-0.23, 0.2), (-0.245, 0.09),
        (-0.31, 0.05), (-0.37, 0.02)]
LOWER_SHIFT = [(0.0, 0.0), (0.01, -0.005), (-0.02, 0.01), (0.01, -0.012), (0.0, 0.0), (0.0, 0.0),
               (0.0, 0.008), (0.01, 0.0), (0.0, 0.0), (0.015, -0.01), (0.0, 0.006), (0.0, 0.0),
               (0.0, 0.0), (0.0, 0.0)]


def _outline():
    upper = HALF + [(-0.4, 0.0)]
    lower = [(x + dx, -(z + dz)) for (x, z), (dx, dz) in zip(reversed(HALF), reversed(LOWER_SHIFT))]
    return upper + lower[1:] if lower[0] == upper[-1] else upper + lower


def build():
    hide = profile_solid("peau", _outline(), lambda _: 0.012,
                         [flat_material("peau_fourrure", FUR)], lambda _: 0,
                         cuts_x=(-0.3, -0.2, -0.1, 0.0, 0.1, 0.2, 0.28), chamfer=0.004)
    roughen(hide, 0.006, seed=4)  # bord ondulé, peau qui ne tombe pas à plat
    two_tone(hide, FUR, FUR_DARK, "peau_fourrure", dark_side=(0.3, 0.8, -0.5), threshold=0.1,
             spread=0.06, seed=4)
    blotches(hide, flat_material("peau_dos", BACK), count=5, radius=0.05, seed=7)
    blotches(hide, flat_material("peau_cuir", LEATHER), count=3, radius=0.025, seed=11)
    return posed([hide], (90, 0, 0), (0, 0, 0.006))
