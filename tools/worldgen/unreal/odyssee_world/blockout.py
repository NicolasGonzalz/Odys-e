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


@dataclass
class Box:
    label: str
    center: tuple  # (x, y, z) local, en mètres
    size: tuple  # (longueur x, largeur y, hauteur z), en mètres
    color: tuple
    shape: str = "cube"
    emissive: float = 0.0
    collision: bool = True


@dataclass
class Light:
    label: str
    center: tuple
    color: tuple
    radius_m: float


# --- Centre de stase ---------------------------------------------------------------------------
# Salle 16 x 12 m, haute de 4 m, six capsules ; couloir de 20 m vers la sortie ; le tout recouvert
# d'un tertre de terre pour donner l'impression d'émerger de sous terre.

ROOM_X, ROOM_Y, ROOM_H = 16.0, 12.0, 4.0
CORRIDOR_L, CORRIDOR_W, CORRIDOR_H = 20.0, 4.0, 3.0
FLOOR = 0.3  # le sol du bunker dépasse un peu du terrain (évite le scintillement)


def stasis_center():
    boxes = _room() + _corridor() + _mound() + _capsules()
    lights = [Light("Lumière — salle des capsules", (0, 0, FLOOR + 3.5), (0.6, 0.8, 1.0), 14),
              Light("Lumière — couloir", (_corridor_mid(), 0, FLOOR + 2.6), (1.0, 0.85, 0.6), 14)]
    player = (-2.0, 0.0, FLOOR + 1.0)
    return boxes, lights, player


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
    end = x + CORRIDOR_L / 2
    return [
        Box("Couloir — sol", (x, 0, FLOOR - 0.25), (CORRIDOR_L, CORRIDOR_W + 2, 0.5), CONCRETE),
        Box("Couloir — plafond", (x, 0, FLOOR + h + 0.5), (CORRIDOR_L, CORRIDOR_W + 2, 1), CONCRETE),
        Box("Couloir — mur gauche", (x, -w - 0.5, FLOOR + h / 2), (CORRIDOR_L, 1, h), CONCRETE),
        Box("Couloir — mur droit", (x, w + 0.5, FLOOR + h / 2), (CORRIDOR_L, 1, h), CONCRETE),
        Box("Sortie — montant gauche", (end + 0.5, -w - 1, FLOOR + 2), (1, 1.5, 4), CONCRETE),
        Box("Sortie — montant droit", (end + 0.5, w + 1, FLOOR + 2), (1, 1.5, 4), CONCRETE),
        Box("Sortie — fronton", (end + 0.5, 0, FLOOR + 4.25), (1, CORRIDOR_W + 4, 0.5), BEACON),
    ]


def _mound():
    """Tertre de terre autour et au-dessus du bunker et du couloir (jamais à l'intérieur)."""
    hx, hy = ROOM_X / 2 + 1, ROOM_Y / 2 + 1  # faces extérieures des murs de la salle
    cx, cw = _corridor_mid(), CORRIDOR_W / 2 + 1  # couloir : centre, face extérieure des murs
    boxes = [
        Box("Tertre — dessus salle", (0, 0, FLOOR + ROOM_H + 1.6), (ROOM_X + 8, ROOM_Y + 8, 1.5),
            EARTH),
        Box("Tertre — arrière", (-hx - 3, 0, 2.6), (6, ROOM_Y + 12, 5.2), EARTH),
        Box("Tertre — dessus couloir", (cx, 0, FLOOR + CORRIDOR_H + 1.6),
            (CORRIDOR_L, CORRIDOR_W + 8, 1.5), EARTH),
    ]
    for sign, side in ((-1, "gauche"), (1, "droite")):
        boxes += [
            Box(f"Tertre — salle {side}", (0, sign * (hy + 3), 2.6), (ROOM_X + 8, 6, 5.2), EARTH),
            Box(f"Tertre — salle {side} (pente)", (0, sign * (hy + 7.5), 1.2),
                (ROOM_X + 4, 3, 2.4), EARTH),
            Box(f"Tertre — couloir {side}", (cx, sign * (cw + 2.5), 2.1), (CORRIDOR_L, 5, 4.2),
                EARTH),
        ]
    return boxes


def _capsules():
    boxes = []
    for index, (x, y) in enumerate([(x, y) for x in (-5.0, -1.5, 2.0) for y in (-4.0, 4.0)]):
        boxes.append(Box(f"Capsule de stase {index + 1}", (x, y, FLOOR + 1.1), (1.2, 1.2, 2.2),
                         CAPSULE, shape="cylinder", emissive=0.4))
    return boxes


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
