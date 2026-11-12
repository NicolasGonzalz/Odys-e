"""Couteau : l'arme et l'outil du début (dépecer, couper des fibres, se défendre).

Référence : art/concepts/objets/outils-et-ressources.png (les deux couteaux).
Lame à dos droit et pointe relevée, fil plus clair, taches de rouille ; petite garde ; manche en
bois à deux rivets. Debout, pointe en haut, tranchant vers -x. Longueur 0,26 m.
"""
from lowpoly.materials import flat_material
from lowpoly.profile import profile_solid
from lowpoly.shading import two_tone
from lowpoly.shapes import bevel, box, cylinder
from lowpoly.tool_parts import TOOL_COLORS, haft, material, rust_spots

ICON = True
VIEW = (0.0, -1.0, 0.15)
PREVIEW_ROTATION_DEG = (0, 30, 0)

HANDLE = 0.11
BLADE_BASE = HANDLE + 0.012
EDGE_BAND = 0.006   # largeur du fil (plus clair)

# Lame vue de face (x, z) : dos droit à droite, tranchant courbé à gauche qui remonte en pointe.
BLADE = [(0.014, BLADE_BASE), (0.014, BLADE_BASE + 0.1), (0.008, BLADE_BASE + 0.135),
         (-0.004, BLADE_BASE + 0.155), (-0.016, BLADE_BASE + 0.12), (-0.024, BLADE_BASE + 0.07),
         (-0.022, BLADE_BASE)]


def _thickness(x):
    """Épaisse au dos (x = 0,014), fine au tranchant (x = -0,024)."""
    t = (x + 0.024) / 0.038
    return 0.0012 + 0.0038 * max(0.0, min(1.0, t))


def _blade():
    edge_limit = -0.024 + EDGE_BAND
    blade = profile_solid("lame", BLADE, _thickness, [material("metal")], lambda _: 0,
                          cuts_x=(edge_limit, -0.004, 0.004), chamfer=0.0008)
    two_tone(blade, TOOL_COLORS["metal_clair"], TOOL_COLORS["metal"], "couteau_lame",
             dark_side=(0.3, 0.6, -0.4), threshold=0.6, seed=2)  # lame claire, tranches sombres
    blade.data.materials.append(material("metal_clair"))
    for polygon in blade.data.polygons:
        if polygon.center.x < edge_limit:
            polygon.material_index = len(blade.data.materials) - 1
    spots = rust_spots("rouille", [(0.006, BLADE_BASE + 0.03, 0.005)],
                       lambda x: _thickness(x) / 2, seed=3)
    return [blade] + spots


def build():
    parts, _, _ = haft("manche", HANDLE, 0.013, 0.012, bend=0.002, seed=6, bare=(0.1, 0.9))
    for i, z in enumerate((0.035, 0.08)):
        for side in (-1, 1):
            parts.append(cylinder(f"rivet_{i}_{side}", 0.0035, 0.003, material("metal_clair"),
                                  vertices=6, location=(0.0, side * 0.0115, z),
                                  rotation_deg=(90, 0, 0)))
    guard = box("garde", (0.042, 0.016, 0.01), flat_material("couteau_garde",
                                                               TOOL_COLORS["metal_sombre"]),
                location=(-0.002, 0, HANDLE + 0.005))
    return parts + [bevel(guard, 0.002)] + _blade()
