"""Flammes : langues de feu low-poly, communes au feu de camp, à la torche et à la forge.

Un contour ondulé et pointu en deux plans croisés, pour se voir de tous les côtés. Elles émettent
de la lumière ; dans Unreal, des particules (Niagara) les remplaceront ou s'y ajouteront.
"""
from .finish import posed
from .materials import flat_material
from .profile import profile_solid

ORANGE = (1.0, 0.45, 0.10)
AMBER = (1.0, 0.62, 0.16)
YELLOW = (1.0, 0.86, 0.42)


def flame(name, color, height, width, angle=0.0, offset=(0.0, 0.0, 0.0), tilt=0.0):
    """Une langue de flamme posée en `offset` (sa base), tournée de `angle` degrés autour de la
    verticale et penchée de `tilt` degrés."""
    w, h = width / 2, height
    outline = [(-w, 0.0), (-w * 0.9, h * 0.3), (-w * 0.35, h * 0.55), (-w * 0.6, h * 0.72),
               (0.0, h), (w * 0.45, h * 0.62), (w * 0.85, h * 0.4), (w, 0.0)]
    material = flat_material(f"flamme_{color}", color, emission=4.0)
    planes = []
    for k, turn in enumerate((0.0, 90.0)):
        plane = profile_solid(f"{name}_{k}", outline, lambda _: 0.012 * min(1.0, width / 0.1),
                              [material], lambda _: 0, chamfer=0.002)
        planes += posed([plane], (tilt, 0, angle + turn), offset)
    return planes
