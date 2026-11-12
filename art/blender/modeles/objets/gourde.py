"""Gourde : l'eau, et donc la soif (docs/VISION.md, survie). Se remplit aux points d'eau.

Référence : art/concepts/objets/outils-et-ressources.png (gourde au centre).
Gourde militaire en métal cabossé, ronde et plate, la moitié basse dans une housse de cuir avec
sa sangle et sa boucle, bouchon vissé. 0,13 × 0,07 m, 0,2 m de haut.
"""
import bmesh

from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.shapes import bevel, blob, box, cylinder, roughen
from lowpoly.tool_parts import TOOL_COLORS, material

ICON = True
VIEW = (0.4, -1.0, 0.35)

RADIUS = (0.065, 0.036, 0.075)    # demi-axes du corps (aplati de face)
CENTER_Z = 0.077
METAL = (0.70, 0.70, 0.70)
METAL_DARK = (0.48, 0.48, 0.49)
LEATHER = (0.55, 0.32, 0.19)
LEATHER_DARK = (0.38, 0.22, 0.13)
COVER_TOP = 0.08                  # la housse monte jusqu'ici


def _body():
    """Corps arrondi : métal en haut, housse de cuir en bas (faces sous COVER_TOP), cabossé."""
    body = blob("gourde", 1.0, flat_material("gourde_metal", METAL), subdivisions=2,
                scale=RADIUS, location=(0, 0, CENTER_Z))
    roughen(body, 0.0018, seed=4)
    two_tone(body, METAL, METAL_DARK, "gourde_metal", dark_side=(0.4, 0.6, -0.5), threshold=0.1,
             seed=4)
    leather = len(body.data.materials)
    body.data.materials.append(flat_material("gourde_cuir", LEATHER))
    body.data.materials.append(flat_material("gourde_cuir_sombre", LEATHER_DARK))
    for polygon in body.data.polygons:
        if polygon.center.z < COVER_TOP - CENTER_Z:
            polygon.material_index = leather + (1 if polygon.normal.dot((0.4, 0.6, -0.5)) > 0.1 else 0)
    return body


def _cap():
    """Goulot et bouchon vissé, avec sa petite chaînette."""
    top = CENTER_Z + RADIUS[2]
    neck = cylinder("goulot", 0.016, 0.02, material("metal_sombre"), vertices=8,
                    location=(0, 0, top + 0.004))
    cap = cylinder("bouchon", 0.021, 0.022, flat_material("gourde_bouchon", METAL_DARK),
                   vertices=8, location=(0, 0, top + 0.022))
    return [neck, bevel(cap, 0.003)]


def _strap():
    """Sangle de cuir qui fait le tour de la housse, et sa boucle sur le devant."""
    dark = flat_material("gourde_sangle", LEATHER_DARK)
    z = COVER_TOP - 0.005
    ring = [(RADIUS[0] * 1.03 * c, RADIUS[1] * 1.12 * s, z)
            for c, s in ((1, 0), (0.7, 0.7), (0, 1), (-0.7, 0.7), (-1, 0), (-0.7, -0.7), (0, -1),
                         (0.7, -0.7), (1, 0))]
    parts = [tube("sangle_tour", ring, [0.006] * len(ring), dark, sides=4)]
    loop = [(-0.03, 0.0, CENTER_Z + RADIUS[2] - 0.004), (-0.012, 0.0, CENTER_Z + RADIUS[2] + 0.03),
            (0.012, 0.0, CENTER_Z + RADIUS[2] + 0.03), (0.03, 0.0, CENTER_Z + RADIUS[2] - 0.004)]
    parts.append(tube("anse", loop, [0.004] * 4, dark, sides=4))  # petite anse près du bouchon
    parts.append(box("boucle", (0.018, 0.006, 0.02), material("metal_clair"),
                     location=(0, -RADIUS[1] * 1.14, z)))
    return parts


def build():
    return [_body()] + _cap() + _strap()
