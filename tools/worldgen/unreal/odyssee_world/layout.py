"""Lecture du plan du monde (world_layout.json) et questions géométriques."""
import json
import math
from pathlib import Path


# --- Géométrie --------------------------------------------------------------------------------

def point_in_polygon(x, y, polygon):
    """Vrai si (x, y) est dans le polygone (méthode du rayon)."""
    inside = False
    j = len(polygon) - 1
    for i, (xi, yi) in enumerate(polygon):
        xj, yj = polygon[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def distance_to_segment(px, py, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    length2 = dx * dx + dy * dy
    t = 0.0 if length2 == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def distance_to_polyline(px, py, points):
    return min(distance_to_segment(px, py, a, b) for a, b in zip(points, points[1:]))


def bounds(polygon):
    xs = [p[0] for p in polygon]
    ys = [p[1] for p in polygon]
    return min(xs), min(ys), max(xs), max(ys)


# --- Le plan ----------------------------------------------------------------------------------

class Layout:
    """Le plan du monde, avec les questions dont la génération a besoin."""

    def __init__(self, data):
        self.data = data
        self.zones = data["zones"]
        self.areas = data["areas"]
        self.places = data["places"]
        self.paths = data["paths"]
        self.river = data["water"]["river"]

    @classmethod
    def load(cls, path):
        with Path(path).open(encoding="utf-8") as f:
            return cls(json.load(f))

    def zone_at(self, x, y):
        for zone_id, zone in self.zones.items():
            if point_in_polygon(x, y, zone["polygon"]):
                return zone_id
        return None

    def region_at(self, x, y):
        """Sous-région (ex. bois_lisiere) si le point y est, sinon la zone."""
        for area_id, area in self.areas.items():
            if point_in_polygon(x, y, area["polygon"]):
                return area_id
        return self.zone_at(x, y)

    def region_polygon(self, region_id):
        region = self.areas.get(region_id) or self.zones[region_id]
        return region["polygon"]

    def distance_to_paths(self, x, y):
        """Distance au bord du chemin le plus proche (négative = sur le chemin)."""
        return min(distance_to_polyline(x, y, p["points"]) - p["width_m"] / 2 for p in self.paths)

    def distance_to_river(self, x, y):
        return distance_to_polyline(x, y, self.river["path"]) - self.river["width_m"] / 2

    def inside_place(self, x, y, margin_m=0.0):
        """Vrai si le point est dans l'emprise d'un lieu (bâtiment, village…)."""
        return any(math.hypot(x - p["pos"][0], y - p["pos"][1]) < p["radius_m"] + margin_m
                   for p in self.places)
