"""CPU regression model for chart-isolated GI filtering; no native build needed."""
import math
import random
import unittest
from pathlib import Path


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def subtract(a, b):
    return tuple(x-y for x, y in zip(a, b))


def length(a):
    return math.sqrt(dot(a, a))


def filter_pixel(values, charts, normals, positions, x, y):
    height, width = len(values), len(values[0])
    center, chart = values[y][x], charts[y][x]
    if not chart:
        return center
    normal, position = normals[y][x], positions[y][x]
    neighbors = [(x+1, y), (x-1, y), (x, y+1), (x, y-1)]
    distances = [length(subtract(positions[j][i], position)) for i, j in neighbors
                 if 0 <= i < width and 0 <= j < height and charts[j][i] == chart]
    footprint = min((d for d in distances if d > 0), default=None)
    if footprint is None:
        return center
    total, weights = center, 1.0
    for dy in range(-3, 4):
        for dx in range(-3, 4):
            i, j = x+dx, y+dy
            if (dx == 0 and dy == 0) or not (0 <= i < width and 0 <= j < height):
                continue
            if charts[j][i] != chart:
                continue
            other = normals[j][i]
            delta = subtract(positions[j][i], position)
            alignment = dot(normal, other)
            if (alignment < 0.9 or length(delta) > footprint*6 or
                    max(abs(dot(delta, normal)), abs(dot(delta, other))) > footprint*0.5):
                continue
            difference = (values[j][i]-center)/max(0.25, 0.5*center)
            weight = math.exp(-(dx*dx+dy*dy)/8)*max(alignment, 0)**32*math.exp(-0.5*difference**2)
            total += values[j][i]*weight
            weights += weight
    return total/weights


class FilterRegression(unittest.TestCase):
    def setUp(self):
        self.values = [[2.0]*16 for _ in range(16)]
        self.charts = [[1]*16 for _ in range(16)]
        self.normals = [[(0, 0, 1)]*16 for _ in range(16)]
        self.positions = [[(x, y, 0) for x in range(16)] for y in range(16)]

    def filtered(self, x, y):
        return filter_pixel(self.values, self.charts, self.normals, self.positions, x, y)

    def test_constant_hdr_and_edges(self):
        self.values = [[12.0]*16 for _ in range(16)]
        for x, y in [(0, 0), (7, 7), (15, 15)]:
            self.assertAlmostEqual(self.filtered(x, y), 12)

    def test_noise_reduction(self):
        rng = random.Random(421)
        self.values = [[2+rng.uniform(-0.4, 0.4) for _ in range(16)] for _ in range(16)]
        before = sum((v-2)**2 for row in self.values for v in row)
        after = sum((self.filtered(x, y)-2)**2 for y in range(16) for x in range(16))
        self.assertLess(after, before/8)

    def test_chart_and_geometry_isolation(self):
        for boundary in ['chart', 'normal', 'position']:
            self.setUp()
            for y in range(16):
                for x in range(8, 16):
                    self.values[y][x] = 4
                    if boundary == 'chart':
                        self.charts[y][x] = 2
                    elif boundary == 'normal':
                        self.normals[y][x] = (1, 0, 0)
                    else:
                        self.positions[y][x] = (x, y, 20)
            self.assertAlmostEqual(self.filtered(7, 8), 2, msg=boundary)
            self.assertAlmostEqual(self.filtered(8, 8), 4, msg=boundary)

    def test_uncovered_and_singleton(self):
        self.charts = [[0]*16 for _ in range(16)]
        self.charts[7][7] = 1
        self.values[7][7] = 9
        self.assertEqual(self.filtered(7, 7), 9)
        self.assertEqual(self.filtered(8, 7), 2)

    def test_native_algorithm_parity(self):
        root = Path(__file__).resolve().parents[3]
        bodies = [(root / ('shaders/src/bake/composition/filter_gi.comp.'+ext)).read_text()
                  .split('    // Filter only the completed estimate;')[1] for ext in ['glsl', 'metal']]
        self.assertEqual(*bodies)


if __name__ == '__main__':
    unittest.main()
