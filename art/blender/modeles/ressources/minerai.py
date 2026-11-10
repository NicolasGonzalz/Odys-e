"""Minerai de métal (ressource) : ce qui tombe d'un filon, à fondre pour obtenir du métal.

Référence : art/concepts/objets/outils-et-ressources.png (découpe : references/minerai.png).
Roche plate gris sombre, couverte de grandes plaques orange rouille (le fer) ; quelques éclats de
métal clair en plus du concept, pour qu'on le reconnaisse d'un coup d'œil. Environ 0,24 m.
"""
from lowpoly.materials import flat_material
from lowpoly.rock import on_surface, rock
from lowpoly.shading import blotches, two_tone
from lowpoly.tool_parts import TOOL_COLORS

ICON = True
VIEW = (0.35, -1.0, 0.7)

ROCK = (0.52, 0.51, 0.50)
ROCK_DARK = (0.28, 0.28, 0.31)
ORE = (0.72, 0.39, 0.20)
ORE_DARK = (0.50, 0.24, 0.14)

# Éclats de métal : (x, y) sur le dessus de la roche, taille (m).
FLECKS = [((0.03, -0.03), 0.014), ((-0.05, -0.005), 0.011), ((0.0, 0.03), 0.01)]


def _ore_patches(obj):
    """Grandes plaques orange (le filon) et leurs bords plus sombres."""
    blotches(obj, flat_material("minerai_filon_sombre", ORE_DARK), count=3, radius=0.04, seed=2)
    blotches(obj, flat_material("minerai_filon", ORE), count=5, radius=0.03, seed=5)
    return obj


def _flecks(stone):
    """Éclats de métal à demi pris dans la roche."""
    metal = flat_material("minerai_eclat", TOOL_COLORS["metal_clair"])
    flecks = []
    for index, ((x, y), size) in enumerate(FLECKS):
        fleck = rock(f"eclat_{index}", (size, size * 0.8, size * 0.6), metal, count=10, dents=0,
                     base=1.0, seed=20 + index)
        fleck.location = on_surface(stone, x, y)
        flecks.append(fleck)
    return flecks


def _lump(name, size, location, seed):
    """Un bloc de roche du minerai, deux tons et plaques de filon."""
    lump = rock(name, size, flat_material("minerai_roche", ROCK), count=26, dents=3, base=0.55,
                facets=0.08, seed=seed)
    lump.location = location
    two_tone(lump, ROCK, ROCK_DARK, "minerai_roche", dark_side=(0.5, 0.4, -0.75), threshold=0.0,
             spread=0.08, seed=seed)
    return _ore_patches(lump)


def build():
    """Deux blocs soudés (la silhouette bosselée du concept), puis les éclats de métal."""
    stone = _lump("minerai", (0.115, 0.09, 0.052), (0, 0, 0), seed=8)
    lobe = _lump("minerai_lobe", (0.07, 0.06, 0.04), (0.075, 0.045, -0.008), seed=9)
    return [stone, lobe, *_flecks(stone)]
