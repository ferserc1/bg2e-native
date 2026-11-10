#!/usr/bin/env python3
"""GGX VNDF numerical/source regression; does not execute production shaders."""
import math
from pathlib import Path
import unittest


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def normalize(a):
    length = math.sqrt(dot(a, a))
    return tuple(x/length for x in a)


def sample(xi, view, roughness):
    # Local frame with the surface normal along +Z.
    alpha = max(roughness*roughness, 1e-4)
    vh = normalize((alpha*view[0], alpha*view[1], view[2]))
    lensq = vh[0]**2+vh[1]**2
    t1 = normalize((-vh[1], vh[0], 0)) if lensq > 1e-8 else (1, 0, 0)
    t2 = cross(vh, t1)
    radius, phi = math.sqrt(xi[0]), 2*math.pi*xi[1]
    x, y = radius*math.cos(phi), radius*math.sin(phi)
    blend = .5*(1+vh[2])
    y = (1-blend)*math.sqrt(max(0, 1-x*x))+blend*y
    z = math.sqrt(max(0, 1-x*x-y*y))
    nh = tuple(x*a+y*b+z*c for a, b, c in zip(t1, t2, vh))
    h = normalize((alpha*nh[0], alpha*nh[1], max(0, nh[2])))
    direction = tuple(2*dot(view, h)*n-v for n, v in zip(h, view))
    nl = direction[2]
    if nl <= 0:
        return direction, 0
    a2 = alpha*alpha
    return direction, 2*nl/(nl+math.sqrt(a2+(1-a2)*nl*nl))


def samples(view, roughness):
    return [sample(((i+.5)/32, (j+.5)/32), view, roughness)
            for i in range(32) for j in range(32)]


class SamplingRegression(unittest.TestCase):
    def test_finite_weights_and_no_fixed_direction_at_grazing(self):
        for elevation in (0.01, 1, 5, 15, 45, 90):
            angle = math.radians(elevation)
            view = (math.cos(angle), 0, math.sin(angle))
            for roughness in (.1, .5, .8, 1):
                rays = samples(view, roughness)
                valid = []
                for ray, weight in rays:
                    self.assertTrue(all(math.isfinite(x) for x in ray))
                    self.assertAlmostEqual(dot(ray, ray), 1, places=10)
                    self.assertTrue(0 <= weight <= 1)
                    if weight:
                        self.assertGreater(ray[2], 0)
                        valid.append(tuple(round(x, 8) for x in ray))
                self.assertGreater(len(valid), 100)
                self.assertEqual(len(valid), len(set(valid)))

    def test_roughness_broadens_normal_incidence(self):
        widths = []
        for roughness in (.1, .3, .5, .8):
            rays = samples((0, 0, 1), roughness)
            widths.append(sum((1-ray[2])*w for ray, w in rays)/sum(w for _, w in rays))
        self.assertEqual(widths, sorted(widths))
        self.assertGreater(widths[-1], 10*widths[0])

    def test_constant_radiance_and_confidence_composition(self):
        rays = samples(normalize((1, 0, .03)), .8)
        total = sum(w for _, w in rays)
        for stride in (1, 2, 7):
            hits = [(ray, w) for i, (ray, w) in enumerate(rays) if i % stride == 0]
            hit_weight = sum(w for _, w in hits)
            radiance = sum(3*w for _, w in hits)/hit_weight
            confidence = hit_weight/total
            # Equal constant hit and environment radiance stays constant.
            self.assertAlmostEqual(radiance*confidence+3*(1-confidence), 3)
            self.assertTrue(0 < confidence <= 1)

    def test_shader_parity_and_no_mirror_fallback(self):
        root = Path(__file__).resolve().parents[3]
        helpers = []
        for extension in ('glsl', 'metal'):
            source = (root / f'shaders/src/draw/lib/rt_reflections.{extension}').read_text()
            start = source.index('reflectionDirection(')
            end = source.index('\n}', source.index('float reflectionSampleWeight(', start))+2
            helpers.append(source[start:end].replace('vec3', 'float3').replace('vec2', 'float2'))
            self.assertNotIn('normalize(reflected+normal*0.1)', source)
            self.assertIn('reflectionDirection(xi,normal,view,samplingRoughness)', source)
            self.assertIn('totalWeight += weight;', source)
            self.assertIn('radiance/hitWeight', source)
            self.assertIn('hitWeight/totalWeight', source)
        self.assertEqual(*helpers)


if __name__ == '__main__':
    unittest.main()
