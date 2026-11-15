"""Plume (ressource) : tombe des oiseaux abattus. Sert à empenner les flèches.

Sans concept : plume de rapace des plaines, brun-gris rayé de bandes sombres, bout clair, posée à
plat. Vexillum asymétrique (un côté plus large) et une fente dans les barbes, pour qu'on la
reconnaisse d'un coup d'œil. ~20 cm. Choix de design à valider.
"""
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.profile import profile_solid

ICON = True
VIEW = (0.15, -1.0, 1.1)

VANE = (0.58, 0.50, 0.42)        # barbes brun-gris
BAR = (0.30, 0.24, 0.19)         # bandes sombres
TIP = (0.93, 0.90, 0.84)         # bout clair
QUILL = (0.95, 0.92, 0.82)       # rachis ivoire

# Bandes sombres (début, fin) le long de la plume, puis le bout clair.
BARS = ((0.055, 0.07), (0.095, 0.11), (0.135, 0.148))
TIP_FROM = 0.165

# Contour des barbes (x, z) : côté large en haut, fente vers x = 0,12, côté étroit en bas.
VANE_OUTLINE = [
    (0.02, 0.0), (0.035, 0.012), (0.06, 0.02), (0.09, 0.024), (0.115, 0.023), (0.118, 0.016),
    (0.124, 0.022), (0.15, 0.018), (0.17, 0.011), (0.182, 0.003), (0.178, -0.004),
    (0.16, -0.01), (0.12, -0.015), (0.08, -0.016), (0.045, -0.012), (0.025, -0.005),
]


def _material_index(x):
    if x >= TIP_FROM:
        return 2
    return 1 if any(start <= x <= end for start, end in BARS) else 0


def _vane():
    cuts = tuple(edge for bar in BARS for edge in bar) + (TIP_FROM,)
    materials = [flat_material("plume_barbes", VANE), flat_material("plume_bande", BAR),
                 flat_material("plume_bout", TIP)]
    return profile_solid("barbes", VANE_OUTLINE, lambda _: 0.0025, materials, _material_index,
                         cuts_x=cuts, chamfer=0.0008)


def _quill():
    """Rachis : dépasse en tuyau nu à la base, s'affine jusqu'au bout."""
    points = [(-0.025, 0, 0), (0.0, 0, 0.0005), (0.06, 0, 0.001), (0.12, 0, 0.0008), (0.18, 0, 0)]
    return tube("rachis", points, [0.0028, 0.0026, 0.002, 0.0015, 0.0008],
                flat_material("plume_rachis", QUILL), sides=5)


def build():
    return posed([_vane(), _quill()], (90, 0, 0), (0, 0, 0.003))
