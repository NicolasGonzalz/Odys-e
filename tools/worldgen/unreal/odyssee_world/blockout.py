"""Formes de blockout (blocs gris) : centre de stase semi-enterré et entrées de grotte.

Chaque forme est décrite dans son repère local, en mètres : x = vers la sortie, y = à droite,
z = vers le haut, origine au sol au centre du lieu. `place` les tourne et les pose dans le monde.
"""
import math
from dataclasses import dataclass

CONCRETE = (0.50, 0.50, 0.48)
EARTH = (0.33, 0.26, 0.18)
ROCK = (0.30, 0.29, 0.27)
VOID = (0.01, 0.01, 0.01)
CAPSULE = (0.70, 0.85, 0.90)
BEACON = (1.00, 0.80, 0.10)
STEEL = (0.20, 0.22, 0.24)
RUST = (0.33, 0.15, 0.08)
WOOD = (0.36, 0.25, 0.14)
WARNING = (0.90, 0.10, 0.05)


@dataclass
class Box:
    label: str
    center: tuple  # (x, y, z) local, en mètres
    size: tuple  # (longueur x, largeur y, hauteur z), en mètres
    color: tuple
    shape: str = "cube"
    emissive: float = 0.0
    collision: bool = True
    yaw: float = 0.0  # rotation propre du bloc (degrés), en plus de celle du lieu


@dataclass
class Light:
    label: str
    center: tuple
    color: tuple
    radius_m: float


# --- Centre de stase ---------------------------------------------------------------------------
# Salle 16 x 12 m, haute de 4 m, six capsules ; couloir de 20 m vers la sortie. Le tout est enterré
# sous une butte (generate_terrain.raise_stasis_hill) : seule la sortie se voit, au fond d'une
# entaille qui s'évase vers le monde, avec façade, murs en aile, porte blindée et esplanade.

ROOM_X, ROOM_Y, ROOM_H = 16.0, 12.0, 4.0
CORRIDOR_L, CORRIDOR_W, CORRIDOR_H = 20.0, 4.0, 3.0
FLOOR = 0.3  # le sol du bunker dépasse un peu du terrain (évite le scintillement)
MOUTH = ROOM_X / 2 + 1 + CORRIDOR_L  # bout du couloir = face de l'entaille (stasis_hill.mouth_m)
FACADE_H = 6.6  # haut de la façade, sous le dessus de la butte
NOTCH, FLARE = 5.0, 0.9  # demi-largeur de l'entaille à la sortie et son évasement (stasis_hill)
WING_YAW = math.degrees(math.atan(FLARE))
WING_STEPS, WING_STEP_M = 4, 4.0  # marches des murs en aile (leur hauteur suit la terre)


def stasis_center(relief):
    """Blocs, lumières et point d'apparition. `relief(a, b)` : hauteur du terrain au-dessus du sol
    du bunker (pour poser l'antenne et les aérations sur la butte, et boucher le trou)."""
    boxes = (_room() + _corridor() + _capsules() + _facade() + _wings(relief) + _door()
             + _esplanade() + _hilltop(relief) + _stasis_plugs(relief))
    lights = [Light("Lumière — salle des capsules", (0, 0, FLOOR + 3.5), (0.6, 0.8, 1.0), 14),
              Light("Lumière — couloir", (_corridor_mid(), 0, FLOOR + 2.6), (1.0, 0.85, 0.6), 14),
              Light("Lumière — lampe de la sortie", (MOUTH + 1.5, 0, FLOOR + 4.0),
                    (1.0, 0.6, 0.3), 12)]
    player = (-2.0, 0.0, FLOOR + 1.0)
    return boxes, lights, player


def stasis_hole():
    """Rectangle (local) à percer dans le Landscape : la face de l'entaille, devant le couloir."""
    return (MOUTH - 5.0, MOUTH + 2.0), CORRIDOR_W / 2 + 0.5


def _corridor_mid():
    return ROOM_X / 2 + 1 + CORRIDOR_L / 2


def _room():
    hx, hy, top = ROOM_X / 2, ROOM_Y / 2, FLOOR + ROOM_H
    door = CORRIDOR_W / 2
    side = (hy + 1 - door) / 2 + door  # centre des deux pans du mur avant
    return [
        Box("Stase — sol", (0, 0, FLOOR - 0.25), (ROOM_X + 2, ROOM_Y + 2, 0.5), CONCRETE),
        Box("Stase — plafond", (0, 0, top + 0.5), (ROOM_X + 2, ROOM_Y + 2, 1), CONCRETE),
        Box("Stase — mur du fond", (-hx - 0.5, 0, FLOOR + ROOM_H / 2), (1, ROOM_Y + 2, ROOM_H),
            CONCRETE),
        Box("Stase — mur gauche", (0, -hy - 0.5, FLOOR + ROOM_H / 2), (ROOM_X + 2, 1, ROOM_H),
            CONCRETE),
        Box("Stase — mur droit", (0, hy + 0.5, FLOOR + ROOM_H / 2), (ROOM_X + 2, 1, ROOM_H),
            CONCRETE),
        Box("Stase — mur avant gauche", (hx + 0.5, -side, FLOOR + ROOM_H / 2),
            (1, hy + 1 - door, ROOM_H), CONCRETE),
        Box("Stase — mur avant droit", (hx + 0.5, side, FLOOR + ROOM_H / 2),
            (1, hy + 1 - door, ROOM_H), CONCRETE),
        Box("Stase — linteau", (hx + 0.5, 0, FLOOR + CORRIDOR_H + (ROOM_H - CORRIDOR_H) / 2),
            (1, CORRIDOR_W, ROOM_H - CORRIDOR_H), CONCRETE),
    ]


def _corridor():
    x, w, h = _corridor_mid(), CORRIDOR_W / 2, CORRIDOR_H
    return [
        Box("Couloir — sol", (x, 0, FLOOR - 0.25), (CORRIDOR_L, CORRIDOR_W + 2, 0.5), CONCRETE),
        Box("Couloir — plafond", (x, 0, FLOOR + h + 0.5), (CORRIDOR_L, CORRIDOR_W + 2, 1), CONCRETE),
        Box("Couloir — mur gauche", (x, -w - 0.5, FLOOR + h / 2), (CORRIDOR_L, 1, h), CONCRETE),
        Box("Couloir — mur droit", (x, w + 0.5, FLOOR + h / 2), (CORRIDOR_L, 1, h), CONCRETE),
    ]


def _capsules():
    boxes = []
    for index, (x, y) in enumerate([(x, y) for x in (-5.0, -1.5, 2.0) for y in (-4.0, 4.0)]):
        boxes.append(Box(f"Capsule de stase {index + 1}", (x, y, FLOOR + 1.1), (1.2, 1.2, 2.2),
                         CAPSULE, shape="cylinder", emissive=0.4))
    return boxes


def _facade():
    """Mur de béton dans la face de l'entaille, percé à la taille du couloir."""
    door, edge, x = CORRIDOR_W / 2, NOTCH + 1.0, MOUTH + 0.5
    lintel = FACADE_H - FLOOR - CORRIDOR_H
    panel = (1, edge - door, FACADE_H)
    return [
        Box("Sortie — façade gauche", (x, -(door + edge) / 2, FACADE_H / 2), panel, CONCRETE),
        Box("Sortie — façade droite", (x, (door + edge) / 2, FACADE_H / 2), panel, CONCRETE),
        Box("Sortie — linteau", (x, 0, FLOOR + CORRIDOR_H + lintel / 2), (1, CORRIDOR_W, lintel),
            CONCRETE),
        Box("Sortie — corniche", (x + 0.3, 0, FACADE_H + 0.2), (1.6, 2 * edge + 1, 0.4), CONCRETE),
        Box("Sortie — bande de danger", (x + 0.55, 0, FLOOR + CORRIDOR_H + 0.25),
            (0.1, CORRIDOR_W, 0.3), BEACON, emissive=1.0, collision=False),
        Box("Sortie — lampe", (x + 0.7, 0, FLOOR + CORRIDOR_H + 1.0), (0.4, 0.8, 0.25), BEACON,
            emissive=3.0, collision=False),
    ]


def _wings(relief):
    """Murs en aile : ils retiennent la terre des deux flancs de l'entaille, en marches. Chaque
    marche monte au niveau de la terre qu'elle retient (lue juste derrière elle)."""
    boxes = []
    for sign, side in ((-1, "gauche"), (1, "droite")):
        ux, uy = math.cos(math.radians(WING_YAW)), sign * math.sin(math.radians(WING_YAW))
        start = (MOUTH + 1.0, sign * (NOTCH + 1.0))
        for index in range(WING_STEPS):
            middle = (index + 0.5) * WING_STEP_M
            center = (start[0] + middle * ux, start[1] + middle * uy)
            height = max(_earth_behind(relief, center, (ux, uy), sign) + 0.3, 0.8)
            boxes.append(Box(f"Mur en aile {side} {index + 1}", (*center, height / 2),
                             (WING_STEP_M, 0.8, height), CONCRETE, yaw=sign * WING_YAW))
    return boxes


def _earth_behind(relief, center, direction, sign):
    """Plus haut point du terrain sur la longueur d'une marche, à 1,2 m derrière le mur."""
    ux, uy = direction
    nx, ny = -uy * sign, ux * sign  # normale vers l'extérieur de l'entaille (vers la terre)
    return max(relief(center[0] + t * ux + 1.2 * nx, center[1] + t * uy + 1.2 * ny)
               for t in (-WING_STEP_M / 2, 0.0, WING_STEP_M / 2))


def _door():
    """Porte blindée : un battant entrouvert devant la façade, l'autre arraché, à plat au sol."""
    door = CORRIDOR_W / 2
    return [
        Box("Porte blindée — battant ouvert", (MOUTH + 1.25, door + 0.7, FLOOR + 1.55),
            (0.4, 3.0, 3.1), STEEL),
        Box("Porte blindée — battant arraché", (MOUTH + 6.0, -3.0, FLOOR + 0.2), (3.1, 2.4, 0.4),
            STEEL, yaw=28.0),
    ]


def _esplanade():
    """Dalles de béton fissurées devant la sortie (elles couvrent aussi le trou du Landscape),
    quelques fûts, une caisse et deux lampadaires."""
    boxes = []
    slabs = ((MOUTH, 0.0, 0.0), (MOUTH + 4.2, 1.5, -0.04), (MOUTH + 8.3, -2.0, 0.03),
             (MOUTH + 12.5, 1.0, -0.06))
    for index, (start, tilt, dz) in enumerate(slabs):
        width = 2 * (NOTCH + (start + 0.5 - MOUTH) * FLARE)  # dans le fond de l'entaille
        boxes.append(Box(f"Esplanade — dalle {index + 1}", (start + 2.0, 0, FLOOR - 0.25 + dz),
                         (4.0, width, 0.5), CONCRETE, yaw=tilt))
    for index, (x, y) in enumerate(((MOUTH + 8.0, 6.5), (MOUTH + 8.8, 7.4), (MOUTH + 7.6, 7.8))):
        boxes.append(Box(f"Fût {index + 1}", (x, y, FLOOR + 0.5), (0.7, 0.7, 1.0), RUST,
                         shape="cylinder"))
    boxes.append(Box("Caisse", (MOUTH + 9.5, -8.0, FLOOR + 0.6), (1.2, 1.2, 1.2), WOOD, yaw=12.0))
    for sign, side in ((-1, "gauche"), (1, "droit")):
        x, y = MOUTH + 14.0, sign * 13.0
        boxes += [Box(f"Lampadaire {side}", (x, y, FLOOR + 2.5), (0.25, 0.25, 5.0), STEEL),
                  Box(f"Lampadaire {side} — tête", (x, y - sign * 0.3, FLOOR + 5.0),
                      (0.3, 0.8, 0.2), BEACON, emissive=2.0 if sign > 0 else 0.0)]
    return boxes


def _hilltop(relief):
    """Sur la butte : une antenne (feu rouge, repère de loin) et deux bouches d'aération."""
    mast = (-4.0, 3.0)
    ground = relief(*mast)
    boxes = [Box("Antenne — mât", (*mast, ground + 5.5), (0.35, 0.35, 12.0), STEEL),
             Box("Antenne — traverse basse", (*mast, ground + 8.5), (0.15, 2.6, 0.15), STEEL),
             Box("Antenne — traverse haute", (*mast, ground + 10.5), (0.15, 1.6, 0.15), STEEL),
             Box("Antenne — feu", (*mast, ground + 11.75), (0.5, 0.5, 0.5), WARNING,
                 emissive=4.0, collision=False)]
    for index, vent in enumerate(((3.0, -4.0), (14.0, 2.5))):
        ground = relief(*vent)
        boxes += [Box(f"Aération {index + 1}", (*vent, ground + 0.9), (1.2, 1.2, 2.4), CONCRETE,
                      shape="cylinder"),
                  Box(f"Aération {index + 1} — chapeau", (*vent, ground + 2.2), (1.7, 1.7, 0.25),
                      STEEL)]
    return boxes


def _stasis_plugs(relief):
    """Bouchons au-dessus du couloir, derrière la façade (comme les grottes, voir _plugs)."""
    tunnel = (CORRIDOR_L, CORRIDOR_W, FLOOR + CORRIDOR_H)
    plugs = _plugs("Sortie", tunnel, CORRIDOR_W / 2 + 1.0, lambda a, b: relief(a + MOUTH, b))
    kept = [box for box in plugs if box.center[0] <= 0.5]  # devant : l'entaille, à ciel ouvert
    for box in kept:
        box.center = (box.center[0] + MOUTH, *box.center[1:])
    return kept


# --- Grottes ----------------------------------------------------------------------------------
# Repère local : l'entrée en x = 0, la grotte s'enfonce vers les x négatifs ; z = 0 au sol de la
# grotte. Un tunnel (longueur, largeur, hauteur) mène à une salle ; parois de `wall` m d'épaisseur.

DIRT = (0.24, 0.21, 0.17)
LOOT = (0.95, 0.70, 0.20)


def cave_interior(label, tunnel, chamber, wall):
    """Sol, parois et plafond du tunnel et de la salle, point de loot et lumières."""
    return (_tunnel_boxes(label, tunnel, wall) + _chamber_boxes(label, tunnel, chamber, wall)
            + [Box(f"{label} — point de loot", (-tunnel[0] - chamber[0] + 2.5, 0, 0.45),
                   (1.2, 0.8, 0.8), LOOT, emissive=1.5)],
            [Light(f"{label} — lumière tunnel", (-tunnel[0] / 2, 0, tunnel[2] - 0.6),
                   (1.0, 0.75, 0.5), tunnel[0]),
             Light(f"{label} — lumière salle", (-tunnel[0] - chamber[0] / 2, 0, chamber[2] - 0.8),
                   (0.7, 0.8, 1.0), max(chamber[:2]) * 1.2)])


def _tunnel_boxes(label, tunnel, wall):
    length, width, height = tunnel
    side = width / 2 + wall / 2
    return [
        # Le sol déborde de 3 m devant l'entrée : le trou du Landscape va jusqu'à +2 m (cave_hole),
        # il ne doit jamais rester de vide sous les pieds.
        Box(f"{label} — sol du tunnel", ((3 - length) / 2, 0, -0.2),
            (length + 3, width + 2 * wall + 2, 0.5), DIRT),
        Box(f"{label} — paroi gauche", (-length / 2 + 0.25, -side, height / 2),
            (length + 0.5, wall, height), ROCK),
        Box(f"{label} — paroi droite", (-length / 2 + 0.25, side, height / 2),
            (length + 0.5, wall, height), ROCK),
        Box(f"{label} — voûte du tunnel", (-length / 2 + 0.5, 0, height + 0.75),
            (length + 1, width + 2 * wall + 1, 1.5), ROCK),
    ]


def _chamber_boxes(label, tunnel, chamber, wall):
    length, width, height = chamber
    x = -tunnel[0] - length / 2  # centre de la salle
    front = -tunnel[0] - wall / 2
    opening = tunnel[1] / 2
    pier = width / 2 - opening  # largeur de chaque pan du mur d'entrée de la salle
    outer = (length + 2 * wall, width + 2 * wall)
    return [
        Box(f"{label} — sol de la salle", (x, 0, -0.2), (*outer, 0.5), DIRT),
        Box(f"{label} — plafond de la salle", (x, 0, height + 0.75), (*outer, 1.5), ROCK),
        Box(f"{label} — paroi gauche de la salle", (x, -width / 2 - wall / 2, height / 2),
            (outer[0], wall, height), ROCK),
        Box(f"{label} — paroi droite de la salle", (x, width / 2 + wall / 2, height / 2),
            (outer[0], wall, height), ROCK),
        Box(f"{label} — fond de la salle", (x - length / 2 - wall / 2, 0, height / 2),
            (wall, width, height), ROCK),
        Box(f"{label} — entrée de la salle (gauche)", (front, -opening - pier / 2, height / 2),
            (wall, pier, height), ROCK),
        Box(f"{label} — entrée de la salle (droite)", (front, opening + pier / 2, height / 2),
            (wall, pier, height), ROCK),
        Box(f"{label} — entrée de la salle (dessus)",
            (front, 0, tunnel[2] + (height - tunnel[2]) / 2),
            (wall, tunnel[1], height - tunnel[2]), ROCK),
    ]


def _beacon(label, tunnel):
    return Box(f"{label} — balise", (3, tunnel[1] / 2 + 3, 6), (0.4, 0.4, 12), BEACON,
               emissive=2.0, collision=False)


PLUG_FROM_M, PLUG_TO_M = -6.0, 3.0  # le trou (cave_hole) et la rangée de points du Landscape autour


def _plugs(label, tunnel, half, relief):
    """Bouchons de roche au-dessus de l'entrée, par tranches de 1 m le long du tunnel.

    Le Landscape est percé au-dessus de la voûte (cave_hole), et le pinceau retire tous les
    triangles qui touchent le trou : la fosse va donc 1 m plus loin que le rectangle. Là où le relief
    monte plus haut que la voûte (une butte, une falaise), il resterait une fente ouverte dans la
    masse, jusqu'au vide sous le terrain. Chaque tranche monte donc jusqu'au point le plus haut du
    relief au-dessus d'elle. `relief(a, b)` : hauteur du terrain au-dessus du sol de la grotte."""
    width = 2 * half + 3  # le trou ± 1 m de triangles retirés, ± la diagonale d'une case en biais
    boxes = []
    a = PLUG_FROM_M
    while a < PLUG_TO_M:
        top = _highest(relief, a - 1, a + 2, width / 2 + 1) + 0.3
        if a + 1 <= 1.6:  # ancien couvercle : au moins voûte + 2,7 m (haut de la falaise)
            top = max(top, tunnel[2] + 2.7)
        bottom = tunnel[2] + 1.5 if a + 1 <= 0.5 else tunnel[2] - 0.25  # sur la voûte, sinon l'auvent
        if top > bottom:
            boxes.append(Box(f"{label} — bouchon {a:+.0f} m", (a + 0.5, 0, (top + bottom) / 2),
                             (1.0, width, top - bottom), ROCK))
        a += 1
    return boxes


def _highest(relief, a_from, a_to, half_width, step=0.5):
    samples = [relief(a_from + i * step, -half_width + j * step)
               for i in range(int((a_to - a_from) / step) + 1)
               for j in range(int(2 * half_width / step) + 1)]
    return max(samples)


def cave(label, shapes, kind, relief):
    """Grotte cachée dans le relief (`kind` : montagne ou butte, voir cave_shapes) : l'intérieur,
    un auvent de roche sur l'entrée, des bouchons dans le trou du Landscape et une balise. Le relief
    autour (falaise, butte, rochers) vient de generate_terrain.py ; `relief(a, b)` le lit."""
    tunnel, chamber, wall = shapes[kind]["tunnel"], shapes[kind]["chamber"], shapes["wall_m"]
    boxes, lights = cave_interior(label, tunnel, chamber, wall)
    half = tunnel[1] / 2 + wall
    boxes += _plugs(label, tunnel, half, relief)
    boxes += [
        Box(f"{label} — auvent", (0.8, 0, tunnel[2] + 1.0), (1.6, 2 * half + 3, 2.5), ROCK),
        Box(f"{label} — rocher gauche", (0.8, -half - 0.6, tunnel[2] / 2 + 0.5),
            (1.6, 1.8, tunnel[2] + 1), ROCK),
        Box(f"{label} — rocher droit", (0.8, half + 0.6, tunnel[2] / 2 + 0.5),
            (1.6, 1.8, tunnel[2] + 1), ROCK),
        _beacon(label, tunnel),
    ]
    return boxes, lights


def cave_hole(shapes, kind, hole_length_m):
    """Rectangle (local) à percer dans le Landscape : là où le tunnel sort du relief (la face de
    la falaise). Plus étroit que les parois et la voûte : le bord en escalier du trou reste caché."""
    tunnel, wall = shapes[kind]["tunnel"], shapes["wall_m"]
    # Large marge le long de l'axe : les triangles de la face de la falaise (1 m) doivent être
    # entièrement dans le trou, sinon des pointes de terrain restent dans l'ouverture. Derrière la
    # face, la voûte du tunnel prend le relais.
    return (-max(hole_length_m, 5.0), 2.0), tunnel[1] / 2 + wall / 2


# --- Pose dans le monde -----------------------------------------------------------------------

def place(local, origin, yaw_deg):
    """Point local → monde (m) : rotation de `yaw_deg` autour de z, puis translation."""
    yaw = math.radians(yaw_deg)
    x, y, z = local
    return (origin[0] + x * math.cos(yaw) - y * math.sin(yaw),
            origin[1] + x * math.sin(yaw) + y * math.cos(yaw),
            origin[2] + z)
