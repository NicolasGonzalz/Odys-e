"""Arbalète de survivant : bricolée avec ce qu'on trouve dans les ruines.

Référence : art/concepts/objets/outils-et-ressources.png (découpe : references/arbalete.png).
Ce qui vient du concept : crosse en bois massive, poignée bandée de tissu, mécanisme en métal gris
sur le côté, rainure et projectile sur le dessus, arc en métal rouillé aux bouts relevés, corde beige.
Ajouts de design (à valider) : l'arc est une lame de ressort de voiture (deux lames superposées),
serrée dans une bride et ligaturée ; étrier à l'avant pour la tendre au pied ; tire la flèche universelle (armes/fleche, même munition
que l'arc), qui n'est pas incluse dans ce modèle ;
pontet sous la détente ; plaques rivetées.

Modèle couché, pointé vers +x, pivot au pied. Longueur 0,92 m, envergure de l'arc 0,62 m.
"""
from lowpoly.materials import flat_material
from lowpoly.organic import band, tube
from lowpoly.profile import profile_solid
from lowpoly.shading import blotches, two_tone
from lowpoly.shapes import bevel, box, cylinder, wedge
from lowpoly.tool_parts import TOOL_COLORS, material
from lowpoly.wood import grain_strokes, streaked

ICON = True
VIEW = (-0.3, -0.75, 1.0)   # de haut et de trois quarts, comme sur le concept
ICON_VIEW = VIEW

STRING = (0.80, 0.72, 0.55)

# Crosse vue de côté (x, z) : talon massif qui plonge (comme une crosse de fusil), poignée plus
# fine, long fût jusqu'à l'avant.
STOCK = [(0.0, -0.038), (0.0, 0.058), (0.07, 0.068), (0.19, 0.094), (0.25, 0.110), (0.80, 0.113),
         (0.825, 0.104), (0.825, 0.074), (0.44, 0.07), (0.37, 0.06), (0.27, 0.058), (0.2, 0.036),
         (0.12, -0.02), (0.03, -0.042)]
BUTT_THICKNESS = 0.052
FORE_THICKNESS = 0.046
STOCK_TOP = 0.113

# Où poser la flèche (son encoche) dans la rainure, dans le repère de construction ; le modèle
# exporté est recentré (pivot au pied) : ARROW_REST sert de repère pour le socket Unreal.
ARROW_REST = (0.354, 0.0, 0.1185)
LATCH_X = 0.35                 # noix : là où la corde est retenue
PROD_X, PROD_Z = 0.775, 0.095  # milieu de l'arc
PROD_SPAN = 0.33               # demi-envergure
PROD_SWEEP = 0.075             # recul des bouts de l'arc (corde tendue)
LEAF_THICKNESS = 0.009         # une lame de ressort : large à plat, fine en hauteur


def _thickness(x):
    return BUTT_THICKNESS if x < 0.2 else FORE_THICKNESS


# --- Crosse ------------------------------------------------------------------------------------

def _stock():
    """Crosse en bois : profil épaissi aux arêtes arrondies, tons en veines le long de x, traits
    du fil sur les flancs."""
    wood = material("bois")
    stock = profile_solid("crosse", STOCK, _thickness, [wood], lambda x: 0,
                          cuts_x=[0.1 * i for i in range(1, 9)] + [0.2], chamfer=0.013, segments=2)
    streaked(stock, TOOL_COLORS["bois"], TOOL_COLORS["bois_sombre"], "arbalete_bois",
             axis=(1, 0, 0), dark_side=(0.2, 0.3, -1.0), threshold=0.2, streak=0.15, seed=6)
    return [stock, *_stock_grain()]


def _stock_grain():
    """Fil du bois sur les deux flancs : longues lignes un peu ondulées."""
    strokes = []
    for side in (-1, 1):
        for x0, x1, z in ((0.05, 0.15, 0.025), (0.06, 0.13, -0.01), (0.46, 0.78, 0.093),
                          (0.52, 0.74, 0.082), (0.30, 0.36, 0.09)):
            y = side * (_thickness(x0) / 2 + 0.0003)
            strokes.append([(x0, y, z), ((x0 + x1) / 2, y, z + 0.004), (x1, y, z - 0.002)])
    return grain_strokes("crosse", strokes, TOOL_COLORS["bois_fil"])


def _grip_wrap():
    """Tissu enroulé autour de la poignée : manchon et bandes croisées inclinées."""
    cloth = material("tissu")
    parts = [box("poignee", (0.09, BUTT_THICKNESS + 0.006, 0.068), cloth,
                 location=(0.225, 0, 0.07), rotation_deg=(0, -20, 0))]
    for index, (x, tilt) in enumerate(((0.19, 18), (0.225, -22), (0.258, 15))):
        parts.append(box(f"poignee_bande_{index}", (0.022, BUTT_THICKNESS + 0.011, 0.075), cloth,
                         location=(x, 0, 0.07), rotation_deg=(0, tilt - 20, 0)))
    for index, part in enumerate(parts):
        bevel(part, 0.012 if index == 0 else 0.009, segments=2)  # tissu enroulé : arrondi
        two_tone(part, TOOL_COLORS["tissu"], TOOL_COLORS["tissu_sombre"], "outil_tissu",
                 threshold=0.3, seed=index)
    return parts


# --- Mécanisme ---------------------------------------------------------------------------------

def _mechanism():
    """Plaques de métal rivetées sur les flancs, noix sur le dessus, détente et pontet dessous."""
    plate_metal, dark = material("metal"), material("metal_sombre")
    parts = []
    for side in (-1, 1):
        y = side * (FORE_THICKNESS / 2 + 0.0015)
        parts.append(bevel(box(f"plaque_{side}", (0.2, 0.004, 0.054), plate_metal,
                               location=(0.33, y, 0.088)), 0.006, segments=2))
        parts += [cylinder(f"rivet_{side}_{i}", 0.0045, 0.004, dark, vertices=6,
                           location=(x, y + side * 0.002, z), rotation_deg=(90, 0, 0))
                  for i, (x, z) in enumerate(((0.25, 0.1), (0.41, 0.1), (0.25, 0.074), (0.41, 0.074)))]
    parts.append(box("noix", (0.026, 0.03, 0.014), dark, location=(LATCH_X, 0, STOCK_TOP + 0.006)))
    parts.append(wedge("detente", 0.045, (0.008, 0.012), (0.006, 0.004), dark,
                       location=(0.31, 0, 0.062), rotation_deg=(0, 70, 0)))
    guard = [(0.27, 0, 0.062), (0.275, 0, 0.032), (0.32, 0, 0.022), (0.365, 0, 0.034),
             (0.375, 0, 0.066)]
    parts.append(tube("pontet", guard, [0.0035] * len(guard), dark, sides=5))
    return parts


def _rail():
    """Rainure sur le dessus du fût. La flèche n'est pas dans ce modèle : c'est l'objet
    armes/fleche (la munition universelle), posé dans la rainure en jeu (voir ARROW_REST)."""
    dark_wood = flat_material("arbalete_rail", TOOL_COLORS["bois_sombre"])
    parts = [box(f"rail_{side}", (0.4, 0.008, 0.008), dark_wood,
                 location=(0.6, side * 0.011, STOCK_TOP + 0.004)) for side in (-1, 1)]
    return parts


# --- Arc ---------------------------------------------------------------------------------------

def _prod_points(span, drop=0.0, steps=14):
    """Ligne de l'arc vue de dessus : les bras reculent vers les bouts, qui se relèvent un peu."""
    points = []
    for i in range(steps + 1):
        y = -span + 2 * span * i / steps
        t = abs(y) / PROD_SPAN
        x = PROD_X - PROD_SWEEP * t ** 1.7 + (0.012 if i in (0, steps) else 0.0)
        points.append((x, y, PROD_Z - drop))
    return points


def _leaf(name, span, drop, width, seed):
    """Une lame de ressort, large et plate, métal clair à deux tons, plaques de rouille."""
    leaf = band(name, _prod_points(span, drop), width, LEAF_THICKNESS, material("metal"))
    two_tone(leaf, TOOL_COLORS["metal"], TOOL_COLORS["metal_sombre"], f"arbalete_{name}",
             dark_side=(-0.3, 0.2, -1.0), threshold=0.1, seed=seed)
    return blotches(leaf, material("rouille"), count=6, radius=0.028, seed=seed)


def _prod():
    """Arc : lame principale et lame courte dessous (ressort de voiture), bride et ligature."""
    parts = [_leaf("arc", PROD_SPAN, 0.0, 0.044, 4),
             _leaf("arc_lame2", 0.18, LEAF_THICKNESS * 0.95, 0.038, 5)]
    parts.append(box("bride", (0.04, FORE_THICKNESS + 0.012, 0.05), material("metal_sombre"),
                     location=(PROD_X - 0.005, 0, PROD_Z)))
    cord = material("corde")
    parts += [box(f"ligature_{i}", (0.009, FORE_THICKNESS + 0.016, 0.042), cord,
                  location=(PROD_X + dx, 0, PROD_Z)) for i, dx in enumerate((-0.035, 0.026))]
    return parts


def _string():
    """Corde tendue des bouts de l'arc jusqu'à la noix."""
    tips = _prod_points(PROD_SPAN)
    latch = (LATCH_X, 0, STOCK_TOP + 0.011)
    rope = flat_material("arbalete_corde", STRING)
    return [tube(f"corde_{i}", [tips[end], latch], [0.0022] * 2, rope, sides=4)
            for i, end in enumerate((0, -1))]


def _stirrup():
    """Étrier en fer à l'avant : on y met le pied pour tendre la corde."""
    loop = [(0.80, -0.045, 0.092), (0.86, -0.056, 0.086), (0.905, -0.03, 0.08), (0.912, 0, 0.08),
            (0.905, 0.03, 0.08), (0.86, 0.056, 0.086), (0.80, 0.045, 0.092)]
    return [tube("etrier", loop, [0.005] * len(loop), material("metal_sombre"), sides=6)]


def build():
    return (_stock() + _grip_wrap() + _mechanism() + _rail() + _prod() + _string()
            + _stirrup())
