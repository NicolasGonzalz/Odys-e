"""Hasard déterministe : même graine → même monde, à chaque génération."""
import math
import random


def seeded_random(*keys):
    """Générateur aléatoire propre à une combinaison de clés (ex. élément + case de grille)."""
    return random.Random(hash_keys(*keys))


def hash_keys(*keys):
    """Hash stable entre deux lancements (contrairement à hash() de Python sur les chaînes)."""
    value = 2166136261
    for key in keys:
        for byte in str(key).encode("utf-8"):
            value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def _lattice(ix, iy, seed):
    """Valeur aléatoire (0 à 1) attachée à un nœud de la grille du bruit."""
    return hash_keys(seed, ix, iy) / 0xFFFFFFFF


def value_noise(x, y, cell_m, seed):
    """Bruit lisse entre 0 et 1 (variations sur environ `cell_m` mètres) : bosquets, clairières."""
    gx, gy = x / cell_m, y / cell_m
    ix, iy = int(math.floor(gx)), int(math.floor(gy))
    fx, fy = gx - ix, gy - iy
    sx, sy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)  # interpolation adoucie
    top = _lattice(ix, iy, seed) * (1 - sx) + _lattice(ix + 1, iy, seed) * sx
    bottom = _lattice(ix, iy + 1, seed) * (1 - sx) + _lattice(ix + 1, iy + 1, seed) * sx
    return top * (1 - sy) + bottom * sy
