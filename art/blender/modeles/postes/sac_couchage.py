"""Sac de couchage : le « lit » du survivant, son point de repos (on y réapparaît après une mort,
docs/VISION.md ; dormir fait passer la nuit). Fabriqué à la main avec du tissu.

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/sac_couchage.png).
Sac kaki usé posé à plat, le haut roulé en oreiller, accrocs plus clairs, deux sangles autour du
rouleau. 1,9 × 0,75 m.
"""
from lowpoly.carpentry import stain
from lowpoly.materials import flat_material
from lowpoly.organic import loft, tube
from lowpoly.shading import two_tone
from lowpoly.shapes import roughen

ICON = True
VIEW = (0.3, -1.0, 1.0)

FABRIC = (0.52, 0.51, 0.42)       # kaki délavé
FABRIC_DARK = (0.38, 0.37, 0.30)
PATCH = (0.62, 0.60, 0.50)         # accrocs, tissu usé jusqu'à la doublure
STRAP = (0.24, 0.20, 0.16)

# Corps du sac, du pied (x = 0) à l'oreiller : (x, hauteur du centre, demi-largeur, demi-épaisseur).
BODY = [(0.0, 0.06, 0.30, 0.06), (0.12, 0.08, 0.36, 0.08), (0.4, 0.09, 0.38, 0.09),
        (0.75, 0.085, 0.37, 0.085), (1.05, 0.095, 0.38, 0.095), (1.35, 0.085, 0.37, 0.085),
        (1.55, 0.07, 0.35, 0.07)]  # rembourré : bosses et creux le long du sac
ROLL_X = 1.68          # axe du rouleau (le haut du sac, roulé)
ROLL_RADIUS = 0.15


def _body():
    bag = loft("sac", BODY, flat_material("sac_tissu", FABRIC), sides=12)
    roughen(bag, 0.014, seed=4)  # le tissu n'est pas tendu : plis et bosses
    return two_tone(bag, FABRIC, FABRIC_DARK, "sac_tissu", dark_side=(0.3, 0.6, -0.6),
                    threshold=0.05, spread=0.1, seed=4)


def _roll():
    """Le haut du sac roulé sur lui-même : un gros rouleau en travers, et son enroulement visible."""
    points = [(ROLL_X, y, ROLL_RADIUS) for y in (-0.37, -0.12, 0.12, 0.37)]
    roll = tube("rouleau", points, [ROLL_RADIUS * 0.95, ROLL_RADIUS, ROLL_RADIUS, ROLL_RADIUS * 0.95],
                flat_material("sac_tissu", FABRIC), sides=10)
    roughen(roll, 0.006, seed=5)
    two_tone(roll, FABRIC, FABRIC_DARK, "sac_rouleau", dark_side=(0.3, 0.6, -0.6), threshold=0.05,
             seed=5)
    spiral = tube("rouleau_spirale", [(ROLL_X + 0.03, -0.375, ROLL_RADIUS + 0.01),
                                      (ROLL_X - 0.04, -0.378, ROLL_RADIUS - 0.04),
                                      (ROLL_X + 0.02, -0.38, ROLL_RADIUS - 0.07)],
                  [0.006] * 3, flat_material("sac_ombre", FABRIC_DARK), sides=4)
    return [roll, spiral] + _straps()


def _straps():
    """Deux sangles en travers du rouleau."""
    straps = []
    for i, y in enumerate((-0.2, 0.2)):
        ring = [(ROLL_X + ROLL_RADIUS * 1.02 * c, y, ROLL_RADIUS + ROLL_RADIUS * 1.02 * s)
                for c, s in ((0, -1), (0.7, -0.7), (1, 0), (0.7, 0.7), (0, 1), (-0.7, 0.7),
                             (-1, 0), (-0.7, -0.7), (0, -1))]
        straps.append(tube(f"sangle_{i}", ring, [0.012] * len(ring),
                           flat_material("sac_sangle", STRAP), sides=4))
    return straps


def _tears():
    """Accrocs : le tissu usé jusqu'à la doublure, plus clair."""
    return [stain(f"accroc_{i}", (x, y, 0.19), r, PATCH, seed=i)
            for i, (x, y, r) in enumerate(((0.45, -0.12, 0.05), (0.95, 0.15, 0.035),
                                           (1.25, -0.2, 0.03)))]


def build():
    return [_body()] + _roll() + _tears()
