"""Hauteurs du terrain, lues dans output/heights_4m.bin (même relief que heightmap.png)."""
import json
import math
from array import array
from pathlib import Path


class Terrain:
    """Hauteur (m) et pente (degrés) en tout point, par interpolation de la grille à 4 m."""

    def __init__(self, output_dir):
        output_dir = Path(output_dir)
        meta = json.loads((output_dir / "terrain_meta.json").read_text(encoding="utf-8"))
        coarse = meta["coarse"]
        self.width = coarse["width"]
        self.height_count = coarse["height"]
        self.step = coarse["step_m"]

        self.values = array("H")
        with (output_dir / coarse["file"]).open("rb") as f:
            self.values.fromfile(f, self.width * self.height_count)
        # Le fichier est en little-endian ; array utilise l'ordre de la machine.
        if array("H", [1]).tobytes() != b"\x01\x00":
            self.values.byteswap()

    def _sample(self, col, row):
        col = min(max(col, 0), self.width - 1)
        row = min(max(row, 0), self.height_count - 1)
        return (self.values[row * self.width + col] - 32768) / 64.0

    def height(self, x, y):
        """Hauteur en mètres au point (x, y), interpolée entre les 4 points voisins."""
        gx, gy = x / self.step, y / self.step
        col, row = int(math.floor(gx)), int(math.floor(gy))
        fx, fy = gx - col, gy - row
        top = self._sample(col, row) * (1 - fx) + self._sample(col + 1, row) * fx
        bottom = self._sample(col, row + 1) * (1 - fx) + self._sample(col + 1, row + 1) * fx
        return top * (1 - fy) + bottom * fy

    def slope_deg(self, x, y):
        """Pente en degrés (différences finies sur ± un pas de grille)."""
        d = self.step
        dx = (self.height(x + d, y) - self.height(x - d, y)) / (2 * d)
        dy = (self.height(x, y + d) - self.height(x, y - d)) / (2 * d)
        return math.degrees(math.atan(math.hypot(dx, dy)))
