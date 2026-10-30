#!/usr/bin/env python3
"""Generate the untextured solar gizmo in BG2 1.2 format using only Python stdlib.

Run from the repository root: python3 scripts/create_sun_light_gizmo.py
The arrow follows local +Z (light propagation); the apparent sun is opposite.
"""

import json
import math
import struct
from pathlib import Path


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def scale(a, value):
    return tuple(x * value for x in a)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def unit(a):
    return scale(a, 1.0 / math.sqrt(sum(x * x for x in a)))


class Mesh:
    def __init__(self, name):
        self.name = name
        self.positions = []
        self.normals = []
        self.indices = []

    def triangle(self, a, b, c):
        normal = cross(sub(b, a), sub(c, a))
        if sum(x * x for x in normal) < 1e-16:
            return
        normal = unit(normal)
        first = len(self.positions) // 3
        for vertex in (a, b, c):
            self.positions.extend(vertex)
            self.normals.extend(normal)
        self.indices.extend((first, first + 1, first + 2))

    def quad(self, a, b, c, d):
        self.triangle(a, b, c)
        self.triangle(a, c, d)

    def tube(self, start, end, radius, end_radius=None, segments=12):
        end_radius = radius if end_radius is None else end_radius
        axis = unit(sub(end, start))
        tangent = unit(cross(axis, (0, 1, 0) if abs(axis[1]) < 0.9 else (1, 0, 0)))
        bitangent = cross(axis, tangent)
        def point(center, size, index):
            angle = math.tau * index / segments
            return add(center, add(scale(tangent, size * math.cos(angle)), scale(bitangent, size * math.sin(angle))))
        for i in range(segments):
            a = point(start, radius, i)
            b = point(start, radius, i + 1)
            c = point(end, end_radius, i + 1)
            d = point(end, end_radius, i)
            self.quad(a, b, c, d)
            self.triangle(start, b, a)
            if end_radius:
                self.triangle(end, d, c)

    def sphere(self, radius, rings=10, segments=20):
        def point(j, i):
            latitude = math.pi * j / rings
            longitude = math.tau * i / segments
            return (radius * math.sin(latitude) * math.cos(longitude),
                    radius * math.sin(latitude) * math.sin(longitude), radius * math.cos(latitude))
        for j in range(rings):
            for i in range(segments):
                self.quad(point(j, i), point(j + 1, i), point(j + 1, i + 1), point(j, i + 1))

    def ring(self, radius, thickness, segments=40, sides=8):
        def point(i, j):
            a, b = math.tau * i / segments, math.tau * j / sides
            r = radius + thickness * math.cos(b)
            return (r * math.cos(a), r * math.sin(a), thickness * math.sin(b))
        for i in range(segments):
            for j in range(sides):
                self.quad(point(i, j), point(i + 1, j), point(i + 1, j + 1), point(i, j + 1))


def integer(value):
    return struct.pack('>i', value)


def string(value):
    encoded = value.encode('utf-8')
    return integer(len(encoded)) + encoded


def array(values, kind):
    return integer(len(values)) + struct.pack(f'>{len(values)}{kind}', *values)


def main():
    body = Mesh('SolarBody')
    body.sphere(0.34)
    body.ring(0.47, 0.025)
    for i in range(12):
        angle = math.tau * i / 12
        ray = (math.cos(angle), math.sin(angle), 0)
        body.tube(scale(ray, 0.55), scale(ray, 0.78), 0.035, 0.015, segments=6)
    direction = Mesh('SolarEmissionDirection')
    direction.tube((0, 0, 0.30), (0, 0, 1.60), 0.065)
    direction.tube((0, 0, 1.60), (0, 0, 2.10), 0.22, 0.0, segments=16)
    meshes = (body, direction)
    materials = [dict(name=mesh.name, **{'class': 'PBRMaterial'}, albedo=[1, 1, 1, 1],
                      metallic=0, roughness=1, isUnlit=True) for mesh in meshes]
    # Tokens are ASCII FourCCs; lengths, float32 arrays and int32 indices are big endian.
    data = bytearray(b'\x00\x01\x02\x00hedr' + integer(len(meshes)))
    data += b'mtrl' + string(json.dumps(materials, separators=(',', ':')))
    data += b'join' + string('{}')
    for mesh in meshes:
        data += b'plstpnam' + string(mesh.name) + b'mnam' + string(mesh.name)
        data += b'varr' + array(mesh.positions, 'f')
        data += b'narr' + array(mesh.normals, 'f')
        uv = [component for _ in range(len(mesh.positions) // 9) for component in (0., 0., 1., 0., 0., 1.)]
        data += b't0ar' + array(uv, 'f')
        data += b't1ar' + array(uv, 'f')
        data += b'indx' + array(mesh.indices, 'i')
    data += b'endf'
    destination = Path(__file__).resolve().parents[1] / 'assets' / 'sun-light-gizmo.bg2'
    destination.write_bytes(data)


if __name__ == '__main__':
    main()
