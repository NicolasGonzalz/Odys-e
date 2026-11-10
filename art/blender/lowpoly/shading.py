"""Tons « peints à la main » : sur les planches, chaque objet a un côté clair et un côté sombre,
et ses facettes varient légèrement de teinte. On le reproduit en répartissant les faces entre
plusieurs matériaux unis (pas de texture : ça reste léger et dans le style low-poly)."""
import random

from mathutils import Vector

from .materials import flat_material


def shade_variants(name, color, spread=0.07, count=3):
    """Quelques nuances autour d'une couleur (plus claire / plus foncée)."""
    variants = []
    for i in range(count):
        factor = 1 + spread * (i - (count - 1) / 2) / max(1, (count - 1) / 2)
        variants.append(flat_material(f"{name}_{i}", tuple(min(1.0, c * factor) for c in color)))
    return variants


def two_tone(obj, light_color, dark_color, name, dark_side=(1.0, -0.2, 0.0), threshold=0.25,
             spread=0.06, seed=0):
    """Côté sombre (faces tournées vers `dark_side`) et côté clair, chacun en 3 nuances
    réparties au hasard : le look facetté et peint des planches."""
    light = shade_variants(f"{name}_clair", light_color, spread)
    dark = shade_variants(f"{name}_sombre", dark_color, spread)
    obj.data.materials.clear()
    for material in light + dark:
        obj.data.materials.append(material)

    rng = random.Random(seed)
    direction = Vector(dark_side).normalized()
    for polygon in obj.data.polygons:
        offset = len(light) if polygon.normal.dot(direction) > threshold else 0
        polygon.material_index = offset + rng.randrange(len(light))
    return obj


def blotches(obj, material, count, radius, seed=0, keep=()):
    """Plaques irrégulières (rouille, filon de minerai) : les faces proches de `count` centres
    tirés au hasard passent au matériau `material`. Les faces dont le matériau est dans `keep`
    (indices) ne changent pas."""
    obj.data.materials.append(material)
    index = len(obj.data.materials) - 1
    rng = random.Random(seed)
    polygons = list(obj.data.polygons)
    centers = [rng.choice(polygons).center.copy() for _ in range(count)]
    for polygon in polygons:
        if polygon.material_index in keep:
            continue
        reach = radius * (0.7 + 0.6 * rng.random())  # bords irréguliers
        if any((polygon.center - c).length < reach for c in centers):
            polygon.material_index = index
    return obj
