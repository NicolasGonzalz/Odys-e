"""Assemble le plan complet du monde : végétation, ville, ronces, lieux, marqueurs, apparition."""
import math

from .city import generate_buildings
from .placement import LabeledActor, Placement, WorldPlan
from .scatter import scatter_barrier, scatter_region

SEED = 2026

# Gabarit provisoire de chaque type de lieu : (largeur, profondeur, hauteur) en mètres,
# exprimé en multiples du rayon du lieu pour la largeur et la profondeur.
STRUCTURES = {
    "batiment": (1.1, 0.9, 12),
    "entree": (0.5, 0.5, 4),
    "barrage": (1.8, 0.25, 14),
    "pont": (1.2, 0.4, 1.5),
}


def build_plan(layout, terrain, catalog, rules, seed=SEED):
    plan = WorldPlan()
    for region_id, region_rules in rules.items():
        for placement in scatter_region(region_id, region_rules, layout, terrain, catalog, seed):
            plan.add(placement)
    for placement in generate_buildings(layout, terrain, seed):
        plan.add(placement)
    for barrier in layout.data["barriers"]:
        for placement in scatter_barrier(barrier, layout, terrain, catalog, seed):
            plan.add(placement)

    plan.labeled += _places(layout, terrain)
    plan.labeled += _markers(layout, terrain)
    plan.spawn = _spawn_point(layout, terrain)
    return plan


# --- Lieux ------------------------------------------------------------------------------------

def _places(layout, terrain):
    labeled = []
    for place in layout.places:
        for index, placement in enumerate(_structure(place, terrain)):
            suffix = f" ({index + 1})" if index else ""
            labeled.append(LabeledActor(f"Lieu — {place['name']}{suffix}", "Odyssee/Lieux",
                                        placement, ["lieu", place["id"]]))
    return labeled


def _structure(place, terrain):
    """Forme provisoire d'un lieu selon son type."""
    x, y = place["pos"]
    radius = place["radius_m"]
    kind = place["kind"]
    if kind == "village":
        return _village(x, y, radius, terrain)
    if kind == "arbre_geant":
        return [Placement("arbre_geant", x, y, terrain.height(x, y), 0.0, (3.0, 3.0, 3.0))]
    if kind not in STRUCTURES:
        return []  # simple repère (ex. col du plateau) : pas de construction
    width, depth, height = STRUCTURES[kind]
    if place["id"] == "centrale_gaia":
        height = 28  # la centrale domine les marais
    return [Placement("batiment", x, y, terrain.height(x, y), 0.0,
                      (width * radius * 2, depth * radius * 2, height))]


def _village(x, y, radius, terrain, houses=6):
    placements = []
    for index in range(houses):
        angle = 2 * math.pi * index / houses
        hx = x + math.cos(angle) * radius * 0.6
        hy = y + math.sin(angle) * radius * 0.6
        yaw = math.degrees(angle) + 90  # les maisons font face à la place du village
        placements.append(Placement("maison", hx, hy, terrain.height(hx, hy), yaw))
    return placements


# --- Boss, artefacts, cocons ------------------------------------------------------------------

def _markers(layout, terrain):
    data = layout.data
    labeled = []
    for boss in data["bosses"]:
        labeled.append(_marker("marqueur_boss", boss["pos"], f"Boss — {boss['name']}",
                               "Odyssee/Boss", ["boss", boss["id"]], terrain))
    for artifact in data["artifacts"]:
        labeled.append(_marker("marqueur_artefact", artifact["pos"],
                               f"Artefact — {artifact['name']}", "Odyssee/Artefacts",
                               ["artefact", artifact["id"]], terrain))
    for index, cocoon in enumerate(data["cocoons"], start=1):
        labeled.append(_marker("cocon", cocoon, f"Cocon {index:02d}", "Odyssee/Cocons",
                               ["cocon", f"cocon_{index:02d}"], terrain))
    return labeled


def _marker(element, pos, label, folder, tags, terrain):
    x, y = pos
    return LabeledActor(label, folder, Placement(element, x, y, terrain.height(x, y)), tags)


def _spawn_point(layout, terrain):
    """Les joueurs apparaissent devant le centre de stase."""
    start = next(p for p in layout.places if p.get("spawn"))
    x, y = start["pos"][0], start["pos"][1] + start["radius_m"] + 10
    return (x, y, terrain.height(x, y) + 2.0)
