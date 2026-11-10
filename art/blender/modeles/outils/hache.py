"""Hache de survie : manche en bois, bandages en tissu, tête en métal gris tachée de rouille.

Référence : art/concepts/objets/outils-et-ressources.png (en haut à gauche).
Modèle debout : manche selon z, tranchant vers +x. Hauteur ~0,75 m.
"""
from lowpoly.materials import flat_material
from lowpoly.shapes import bevel, box, cylinder, wedge

HANDLE_LENGTH = 0.74
HEAD_Z = 0.63  # hauteur du centre de la tête


def _handle():
    handle = cylinder("manche", 0.019, HANDLE_LENGTH, flat_material("bois"), vertices=6,
                      location=(0, 0, HANDLE_LENGTH / 2), radius_top=0.016)
    knob = cylinder("pommeau", 0.025, 0.035, flat_material("bois_sombre"), vertices=6,
                    location=(0, 0, 0.017))
    return [handle, knob]


def _bandage(name, bottom_z, turns, radius):
    """Bandes de tissu enroulées : des anneaux légèrement inclinés, un peu décalés."""
    cloth = flat_material("tissu")
    strips = []
    for turn in range(turns):
        tilt = 9 if turn % 2 == 0 else -9
        strips.append(cylinder(f"{name}_{turn}", radius, 0.03, cloth, vertices=6,
                               location=(0, 0, bottom_z + turn * 0.026),
                               rotation_deg=(tilt, 0, turn * 25)))
    return strips


def _head():
    """Tête : un talon épais à l'arrière, une lame qui s'évase et s'amincit vers l'avant."""
    body = wedge("tete", 0.15, back=(0.056, 0.085), front=(0.018, 0.16),
                 material=flat_material("metal_sombre"), location=(-0.04, 0, HEAD_Z))
    edge = wedge("tranchant", 0.035, back=(0.018, 0.16), front=(0.003, 0.19),
                 material=flat_material("metal"), location=(0.11, 0, HEAD_Z))
    return [bevel(body, 0.005), edge]


def _rust_patches():
    """Taches de rouille sur les flancs de la tête (comme sur la planche)."""
    rust = flat_material("rouille")
    patches = []
    for side in (-1, 1):
        patches.append(box(f"rouille_{side}_a", (0.05, 0.004, 0.04), rust,
                           location=(0.02, side * 0.022, HEAD_Z + 0.025), rotation_deg=(0, 12, 0)))
        patches.append(box(f"rouille_{side}_b", (0.035, 0.004, 0.03), rust,
                           location=(0.06, side * 0.016, HEAD_Z - 0.04), rotation_deg=(0, -8, 0)))
    return patches


def build():
    grip = _bandage("poignee", 0.07, 6, 0.023)
    collar = _bandage("collier", HEAD_Z - 0.13, 3, 0.021)
    return _handle() + grip + collar + _head() + _rust_patches()
