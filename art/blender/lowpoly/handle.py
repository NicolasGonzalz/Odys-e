"""Manche d'outil à partir des tranches mesurées sur l'image (references/<nom>_trace.json).

Chaque tranche : centre x, hauteur z, rayon r et matière (bois, tissu, embout). Ce module lisse
les mesures, les allège pour le low-poly, fait passer le manche caché à travers la tête, et
construit un tube par zone de matière.
"""
from .organic import tube

SMOOTHING = 3   # moyenne glissante sur ± 3 tranches (les pixels sont irréguliers)
STEP = 3        # une tranche lissée sur 3 dans le maillage


def smoothed(slices):
    """Le centre et le rayon lissés : la courbe mesurée, sans le bruit des pixels."""
    result = []
    for i, slice_ in enumerate(slices):
        window = slices[max(0, i - SMOOTHING): i + SMOOTHING + 1]
        result.append({**slice_, "x": sum(s["x"] for s in window) / len(window),
                       "r": sum(s["r"] for s in window) / len(window)})
    return result


def decimated(slices):
    """Une tranche sur STEP, plus toutes celles où la matière change (limites nettes)."""
    kept = [slices[0]]
    for i in range(1, len(slices) - 1):
        if slices[i]["matiere"] != slices[i - 1]["matiere"] or i % STEP == 0:
            kept.append(slices[i])
    return kept + [slices[-1]]


def through_head(slices, z_bottom, z_top, hidden_radius):
    """Sur le dessin, la tête cache le manche : aucune mesure entre z_bottom et z_top.
    On y fait passer le manche, plus fin que la tête, pour qu'il reste invisible."""
    below = [s for s in slices if s["z"] < z_bottom]
    above = [s for s in slices if s["z"] > z_top]
    x_bottom = below[-1]["x"] if below else 0.0
    x_top = above[0]["x"] if above else x_bottom
    inside = [{"x": x_bottom, "z": z_bottom + 0.004, "r": hidden_radius, "matiere": "bois"},
              {"x": x_top, "z": z_top - 0.004, "r": hidden_radius, "matiere": "bois"}]
    return below + inside + above


def prepared(trace, z_bottom, z_top, hidden_radius):
    """Tranches prêtes à construire : triées, lissées, allégées, passage caché dans la tête."""
    slices = sorted(trace["manche"], key=lambda s: s["z"])
    return through_head(decimated(smoothed(slices)), z_bottom, z_top, hidden_radius)


def interpolator(slices, key):
    """Fonction z → valeur (centre « x » ou rayon « r ») entre les tranches."""
    def at(z):
        for low, high in zip(slices, slices[1:]):
            if low["z"] <= z <= high["z"]:
                t = (z - low["z"]) / max(high["z"] - low["z"], 1e-6)
                return low[key] + (high[key] - low[key]) * t
        return slices[0][key] if z < slices[0]["z"] else slices[-1][key]
    return at


def runs(slices):
    """Groupes de tranches consécutives de même matière (tranche de jonction partagée)."""
    groups, current = [], [slices[0]]
    for slice_ in slices[1:]:
        if slice_["matiere"] == current[-1]["matiere"]:
            current.append(slice_)
        else:
            groups.append(current)
            current = [current[-1], slice_]
    groups.append(current)
    return groups


def build(name, slices, material_for, sides=7, flatten=1.0):
    """Un tube par zone de matière. `material_for(matiere)` donne le matériau Blender."""
    parts = []
    for index, group in enumerate(runs(slices)):
        material = group[-1]["matiere"]
        points = [(s["x"], 0.0, s["z"]) for s in group]
        parts.append(tube(f"{name}_{index}_{material}", points, [s["r"] for s in group],
                          material_for(material), sides=sides, flatten=flatten))
    return parts
