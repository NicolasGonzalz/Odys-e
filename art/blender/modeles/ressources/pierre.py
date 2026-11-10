"""Pierre (ressource) : ce qui tombe quand on frappe un rocher à la pioche.

Référence : art/concepts/objets/outils-et-ressources.png (découpe : references/pierre.png).
Caillou trapu à grandes facettes, gris chaud ; dessus clair, flanc droit et dessous sombres,
un creux sur le côté. Environ 0,22 m (tient dans la main).

Choix à valider : la pierre ramassée suit le concept (gris chaud), alors que la pierre taillée des
outils est un gris ardoise plus froid (STYLE.md § 3, pour la distinguer du métal).
"""
from lowpoly.materials import flat_material
from lowpoly.rock import rock
from lowpoly.shading import two_tone

ICON = True
VIEW = (0.35, -1.0, 0.7)   # légèrement de haut, comme sur le concept

STONE = (0.72, 0.67, 0.59)
STONE_DARK = (0.45, 0.43, 0.43)
STONE_TOP = (0.80, 0.75, 0.66)  # facettes du dessus, qui prennent la lumière


def _light_top(obj):
    """Les facettes tournées vers le haut sont plus claires (la lumière des planches vient d'en haut)."""
    obj.data.materials.append(flat_material("pierre_dessus", STONE_TOP))
    top = len(obj.data.materials) - 1
    for polygon in obj.data.polygons:
        if polygon.normal.z > 0.75:
            polygon.material_index = top
    return obj


def build():
    stone = rock("pierre", (0.11, 0.095, 0.1), flat_material("pierre", STONE), count=40,
                 dents=3, facets=0.12, seed=11)
    two_tone(stone, STONE, STONE_DARK, "pierre", dark_side=(0.6, 0.3, -0.75), threshold=0.0,
             spread=0.08, seed=11)
    return [_light_top(stone)]
