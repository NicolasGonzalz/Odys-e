"""Lampe torche : indispensable dans les hangars, les sous-sols et la nuit des Terres noyées.

Référence : art/concepts/ambiances/zone-4-site-des-portes-lieux.jpeg (Les Hangars) :
lampe trapue gris anthracite à facettes, tête plus large, verre jaune lumineux, bouton orange.
Ajouts : bagues de prise en caoutchouc, ruban adhésif de réparation, dragonne.

Couchée, faisceau vers +x. Longueur 0,2 m. Le verre est émissif (il éclaire déjà dans Unreal ;
la vraie lumière sera une SpotLight attachée au bout).
"""
from lowpoly.materials import flat_material
from lowpoly.organic import tube
from lowpoly.shading import two_tone
from lowpoly.shapes import box, cylinder

ICON = True
VIEW = (0.5, -1.0, 0.6)

BODY = (0.38, 0.39, 0.41)
BODY_DARK = (0.24, 0.25, 0.27)
RUBBER = (0.13, 0.13, 0.14)
GLASS = (1.0, 0.86, 0.45)
BUTTON = (0.95, 0.55, 0.15)
TAPE = (0.36, 0.37, 0.36)
AXIS_Z = 0.03

# Profil (x, rayon) : culot, corps, évasement, tête.
PROFILE = [(0.0, 0.017), (0.012, 0.019), (0.12, 0.019), (0.145, 0.026), (0.17, 0.029),
           (0.2, 0.029)]


def _body():
    """Corps à huit pans, deux tons, verre lumineux au bout."""
    body = tube("corps", [(x, 0, AXIS_Z) for x, _ in PROFILE], [r for _, r in PROFILE],
                flat_material("lampe_corps", BODY), sides=8)
    two_tone(body, BODY, BODY_DARK, "lampe_corps", dark_side=(0, 0.3, -1), seed=1)
    glass = cylinder("verre", 0.025, 0.003, flat_material("lampe_verre", GLASS, emission=4.0),
                     vertices=8, location=(0.2005, 0, AXIS_Z), rotation_deg=(0, 90, 0))
    return [body, glass]


def _details():
    """Bagues de caoutchouc, ruban adhésif, bouton orange, dragonne au culot."""
    rubber = flat_material("lampe_caoutchouc", RUBBER)
    parts = [cylinder(f"bague_{i}", 0.0205, 0.008, rubber, vertices=8, location=(x, 0, AXIS_Z),
                      rotation_deg=(0, 90, 0)) for i, x in enumerate((0.03, 0.05, 0.07))]
    parts.append(cylinder("ruban", 0.0203, 0.022, flat_material("lampe_ruban", TAPE), vertices=8,
                          location=(0.103, 0, AXIS_Z), rotation_deg=(0, 82, 0)))
    parts.append(box("bouton", (0.012, 0.01, 0.006), flat_material("lampe_bouton", BUTTON,
                     emission=0.5), location=(0.13, 0, AXIS_Z + 0.022)))
    parts.append(tube("dragonne", [(-0.002, 0, AXIS_Z), (-0.02, 0.006, AXIS_Z - 0.012),
                                   (-0.03, -0.004, AXIS_Z - 0.02), (-0.012, -0.006, AXIS_Z - 0.006)],
                      [0.0025] * 4, rubber, sides=4))
    return parts


def build():
    return _body() + _details()
