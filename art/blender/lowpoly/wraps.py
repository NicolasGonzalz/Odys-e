"""Bandages autour d'un manche : rubans croisés en X et bord effiloché, plaqués contre le bois.

`center_x(z)` donne le centre du manche à la hauteur z (les manches sont légèrement courbés),
`radius_at(z)` son rayon. Les rubans sont de fines bandes posées à la surface, pas des tubes.
"""
import math

import bpy

from .outline import without_outline

GAP = 0.0015       # le tissu est posé juste au-dessus du bois (m)
THICKNESS = 0.002  # épaisseur du tissu (m)


def _surface_point(center_x, radius_at, z, angle, lift, flatten=1.0):
    """Point posé sur un manche ovale (aplati selon y par `flatten`, comme organic.tube)."""
    r = radius_at(z)
    return (center_x(z) + (r + lift) * math.cos(angle), (r * flatten + lift) * math.sin(angle), z)


def _mesh(name, vertices, faces, material):
    """Pièce de tissu fine (2 mm) : sans contour noir, sinon sa coque ressort en pointillés noirs
    à travers le bandage. Le manchon dessous porte le contour de la silhouette."""
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material)
    return without_outline(obj)


def _band(name, edges, material):
    """Bande épaisse à partir de paires de points (bas, haut) le long de son parcours.

    Chaque pair est posée sur la surface ; on ajoute la même paire un peu plus loin du centre
    (l'épaisseur du tissu), puis on relie le tout en un ruban fermé.
    """
    vertices, faces = [], []
    for (low_in, high_in), (low_out, high_out) in edges:
        vertices += [low_in, high_in, high_out, low_out]
    for i in range(len(edges) - 1):
        a, b = 4 * i, 4 * (i + 1)
        for k in range(4):
            faces.append((a + k, a + (k + 1) % 4, b + (k + 1) % 4, b + k))
    faces += [(3, 2, 1, 0), tuple(4 * (len(edges) - 1) + k for k in range(4))]
    return _mesh(name, vertices, faces, material)


def ribbon(name, center_x, radius_at, z_bottom, z_top, material, turns, phase, direction,
           width, flatten=1.0):
    """Ruban de tissu enroulé en spirale, à plat contre le manche."""
    steps = max(10, int(turns * 14))
    edges = []
    for i in range(steps + 1):
        t = i / steps
        z = z_bottom + width / 2 + (z_top - z_bottom - width) * t
        angle = phase + direction * 2 * math.pi * turns * t
        inner = [_surface_point(center_x, radius_at, z + dz, angle, GAP, flatten) for dz in (-width / 2, width / 2)]
        outer = [_surface_point(center_x, radius_at, z + dz, angle, GAP + THICKNESS,
                                flatten)
                 for dz in (-width / 2, width / 2)]
        edges.append((tuple(inner), tuple(outer)))
    return _band(name, edges, material)


def crossed_wrap(name, center_x, radius_at, z_bottom, z_top, material, turns=1.2, width=0.022,
                 flatten=1.0):
    """Deux rubans de sens contraires : le croisement en X des bandages."""
    return [ribbon(f"{name}_ruban_{index}", center_x, radius_at, z_bottom, z_top, material,
                   turns, phase, direction, width, flatten)
            for index, (phase, direction) in enumerate(((0.0, 1), (math.pi, -1)))]


def frayed_edge(name, center_x, radius_at, z, material, teeth=9, length=0.022, seed=0,
                flatten=1.0):
    """Bord déchiré sous un bandage : des dents de tissu de longueurs inégales, plaquées."""
    vertices, faces = [], []
    for i in range(teeth):
        a0 = 2 * math.pi * i / teeth
        a1 = 2 * math.pi * (i + 1) / teeth
        tip = length * (0.45 + 0.55 * ((i * 37 + seed * 13) % 9) / 8)  # longueurs variées
        for lift in (GAP, GAP + THICKNESS):
            base = len(vertices)
            vertices += [_surface_point(center_x, radius_at, z, a0, lift, flatten),
                         _surface_point(center_x, radius_at, z - tip, (a0 + a1) / 2, lift, flatten),
                         _surface_point(center_x, radius_at, z, a1, lift, flatten)]
            faces.append((base, base + 1, base + 2) if lift == GAP else (base + 2, base + 1, base))
    return [_mesh(f"{name}_franges", vertices, faces, material)]
