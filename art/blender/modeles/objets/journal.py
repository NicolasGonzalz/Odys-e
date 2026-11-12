"""Journal : le carnet d'un survivant ou d'un opérateur d'avant. Le lire ajoute un indice au journal
du joueur (T049, docs/VISION.md).

Référence : art/concepts/objets/objets-interactifs.jpg (découpe : references/journal.png).
Carnet épais en cuir brun usé, coin écorné, étiquette de papier sur la couverture, rabat à bouton
qui le ferme, pages jaunies qui dépassent, signet rouge. Posé à plat, 0,22 × 0,16 m.
"""
from lowpoly.finish import posed
from lowpoly.materials import flat_material
from lowpoly.profile import profile_solid
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import bevel, box, cylinder
from lowpoly.tool_parts import material

ICON = True
VIEW = (0.3, -1.0, 0.9)

LENGTH, WIDTH = 0.22, 0.16
PAGES = 0.04
COVER = 0.006
LEATHER = (0.62, 0.34, 0.20)
LEATHER_DARK = (0.40, 0.21, 0.13)
WORN = (0.66, 0.42, 0.27)
PAGE = (0.92, 0.84, 0.62)
LABEL = (0.88, 0.80, 0.60)
RIBBON = (0.72, 0.20, 0.14)

# Couverture vue de dessus (x, y) : coin avant droit écorné.
COVER_OUTLINE = [(-LENGTH / 2, -WIDTH / 2), (LENGTH / 2 - 0.025, -WIDTH / 2),
                 (LENGTH / 2, -WIDTH / 2 + 0.02), (LENGTH / 2, WIDTH / 2),
                 (-LENGTH / 2, WIDTH / 2)]


def _cover(name, z, seed):
    """Plat de couverture en cuir : deux tons, zones usées plus claires."""
    flat = [(x, -y) for x, y in COVER_OUTLINE]
    cover = profile_solid(name, flat, lambda _: COVER, [flat_material("journal_cuir", LEATHER)],
                          lambda _: 0, cuts_x=(-0.06, 0.0, 0.06), chamfer=0.002)
    two_tone(cover, LEATHER, LEATHER_DARK, "journal_cuir", dark_side=(0.3, 0.4, -1.0),
             threshold=0.5, seed=seed)  # seul le dessous et les tranches sont sombres
    blotches(cover, flat_material("journal_usure", WORN), count=2, radius=0.04, seed=seed)
    return posed([cover], (90, 0, 0), (0, 0, z))


def _pages():
    """Bloc de pages jaunies, un peu en retrait des couvertures."""
    block = box("pages", (LENGTH - 0.012, WIDTH - 0.01, PAGES), flat_material("pages", PAGE),
                location=(0.004, 0, COVER + PAGES / 2))
    return [bevel(block, 0.003)]


def _spine():
    """Dos arrondi, côté gauche."""
    spine = cylinder("dos", PAGES / 2 + COVER, WIDTH, flat_material("journal_cuir", LEATHER),
                     vertices=10, location=(-LENGTH / 2, 0, COVER + PAGES / 2),
                     rotation_deg=(90, 0, 0))
    return [two_tone(spine, LEATHER, LEATHER_DARK, "journal_dos", seed=3)]


def _strap_and_label():
    """Rabat qui passe du dessus au bord droit, son bouton, l'étiquette et le signet."""
    top = COVER * 2 + PAGES
    leather = flat_material("journal_rabat", LEATHER_DARK)
    parts = [box("rabat_dessus", (0.05, 0.035, 0.004), leather,
                 location=(LENGTH / 2 - 0.025, -0.01, top + 0.002)),
             box("rabat_bord", (0.004, 0.035, top), leather,
                 location=(LENGTH / 2 + 0.002, -0.01, top / 2)),
             cylinder("bouton", 0.008, 0.005, material("metal_clair"), vertices=8,
                      location=(LENGTH / 2 - 0.04, -0.01, top + 0.0045)),
             box("etiquette", (0.08, 0.045, 0.001), flat_material("etiquette", LABEL),
                 location=(-0.02, 0.025, top + 0.0006), rotation_deg=(0, 0, -3)),
             box("signet", (0.012, 0.03, 0.001), flat_material("signet", RIBBON),
                 location=(-0.04, -WIDTH / 2 - 0.012, COVER + 0.004), rotation_deg=(0, 0, 12))]
    return parts


def build():
    return (_cover("couverture_dessous", COVER / 2, 1) + _pages()
            + _cover("couverture_dessus", COVER * 1.5 + PAGES, 2) + _spine() + _strap_and_label())
