#!/usr/bin/env python3
"""P4.2 independent current-neighborhood reference; does not execute shaders."""
from dataclasses import dataclass, replace
from pathlib import Path
import math
import re
import struct

EPSILON = 1e-6
LUMA = (.2126, .7152, .0722)
ROOT = Path(__file__).resolve().parents[3]


@dataclass(frozen=True)
class Receiver:
    token: int = 7
    depth: float = .5
    metadata_depth: float = .5
    # The independently supplied world point represents reconstructed current depth.
    position: tuple = (0, 0, -2)
    encoded_normal: tuple = (.5, .5, 1)
    roughness: float = .5


def gather(pixel, signal_extent, receiver_extent, signal, receiver, kind='scalar',
           center_token=7, center_position=(0, 0, -2), center_normal=(0, 0, 1),
           camera=(0, 0, 0), absolute=.02, relative=.01, cosine=.9, max_roughness=1):
    tolerance = absolute + relative * math.dist(center_position, camera)
    if center_token == 0 or not all(math.isfinite(v) for v in (*center_position, *center_normal, tolerance)) or tolerance < 0:
        return None
    values, colors, visits = [], [], []
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            p = (pixel[0] + dx, pixel[1] + dy)
            if not all(0 <= c < size for c, size in zip(p, signal_extent)):
                continue
            # Exact integer form of floor((p+.5)/signalExtent * receiverExtent).
            r = tuple((2*c+1)*size//(2*extent) for c, size, extent in zip(p, receiver_extent, signal_extent))
            guide = receiver(r)
            if guide.token == 0 or guide.token != center_token:
                continue
            if not all(math.isfinite(v) for v in (guide.depth, guide.metadata_depth, *guide.position, *guide.encoded_normal)):
                continue
            if not 0 <= guide.depth < 1 or not 0 <= guide.metadata_depth < 1 or abs(guide.depth-guide.metadata_depth) > 2e-6:
                continue
            decoded = tuple(2*v-1 for v in guide.encoded_normal)
            n2 = sum(v*v for v in decoded)
            if sum(v*v for v in guide.encoded_normal) <= EPSILON or n2 <= EPSILON:
                continue
            normal = tuple(v/math.sqrt(n2) for v in decoded)
            if math.dist(guide.position, center_position) > tolerance or sum(a*b for a, b in zip(normal, center_normal)) < cosine:
                continue
            sample = signal(p)
            if not all(math.isfinite(v) for v in sample):
                continue
            if kind == 'scalar':
                statistic = min(1, max(0, sample[0]))
                color = (statistic,)*3
            else:
                if kind == 'gi' and sample[3] <= .5:
                    continue
                if kind == 'reflection' and (not math.isfinite(guide.roughness) or guide.roughness > max_roughness or min(1, max(0, sample[3])) <= EPSILON):
                    continue
                color = sample[:3]
                statistic = max(sum(a*b for a, b in zip(color, LUMA)), 0)
            colors.append(color)
            values.append(statistic)
            visits.append((p, r))
    if not values:
        return None
    # fsum is an independent, more accurate reference for float32 shader sums.
    mean = math.fsum(values)/len(values)
    second = math.fsum(v*v for v in values)/len(values)
    return dict(mean=mean, second=second, variance=max(second-mean*mean, 0),
                minimum=tuple(min(c[i] for c in colors) for i in range(3)),
                maximum=tuple(max(c[i] for c in colors) for i in range(3)),
                statistic_min=min(values), statistic_max=max(values),
                count=len(values), variance_valid=len(values) >= 2, visits=visits)


def check_math():
    guide = lambda _: Receiver()
    pixels = {(x, y): ((3*y+x)/8,) for y in range(3) for x in range(3)}
    signal = pixels.__getitem__
    stats = gather((1, 1), (3, 3), (3, 3), signal, guide)
    assert stats['count'] == 9 and stats['mean'] == .5
    assert math.isclose(stats['variance'], 5/48) and stats['minimum'] == (0, 0, 0) and stats['maximum'] == (1, 1, 1)
    corner = gather((0, 0), (3, 3), (3, 3), signal, guide)
    assert corner['count'] == 4 and len(set(p for p, _ in corner['visits'])) == 4
    one = gather((0, 0), (1, 1), (1, 1), lambda _: (.75,), guide)
    assert one['count'] == 1 and not one['variance_valid'] and one['variance'] == 0
    mapped = gather((0, 0), (2, 2), (5, 3), lambda _: (.5,), guide)
    assert [r for _, r in mapped['visits']] == [(1, 0), (3, 0), (1, 2), (3, 2)]
    clamped = gather((0, 0), (2, 1), (2, 1), lambda p: (-5,) if p[0] == 0 else (5,), guide)
    assert clamped['mean'] == .5 and clamped['variance'] == .25
    # Same-token background, inconsistent coverage and different receivers cannot set bounds.
    bad = [replace(Receiver(), token=0), replace(Receiver(), token=8), replace(Receiver(), depth=1),
           replace(Receiver(), metadata_depth=.6), replace(Receiver(), position=(0, 0, -3)),
           replace(Receiver(), encoded_normal=(0, 0, 0)), replace(Receiver(), encoded_normal=(.5, .5, .5)),
           replace(Receiver(), encoded_normal=(1, .5, .5))]
    for nonfinite in (math.nan, math.inf, -math.inf):
        bad.extend((replace(Receiver(), depth=nonfinite), replace(Receiver(), metadata_depth=nonfinite),
                    replace(Receiver(), position=(nonfinite, 0, -2)), replace(Receiver(), encoded_normal=(nonfinite, .5, 1))))
    for invalid in bad:
        assert gather((0, 0), (1, 1), (1, 1), lambda _: (1000,), lambda _: invalid) is None
        filtered = gather((0, 0), (2, 1), (2, 1), lambda p: (1000,) if p[0] else (.25,),
                          lambda p: invalid if p[0] else Receiver())
        assert filtered['count'] == 1 and not filtered['variance_valid'] and filtered['mean'] == .25
    for nonfinite in (math.nan, math.inf, -math.inf):
        assert gather((0, 0), (1, 1), (1, 1), lambda _: (nonfinite,), guide) is None
    black = gather((0, 0), (2, 1), (2, 1), lambda _: (0, 0, 0, 1), guide, kind='gi')
    assert black['variance_valid'] and black['mean'] == 0 and black['maximum'] == (0, 0, 0)
    assert gather((0, 0), (1, 1), (1, 1), lambda _: (50, 50, 50, 0), guide, kind='gi') is None
    assert gather((0, 0), (1, 1), (1, 1), lambda _: (50, 50, 50, 0), guide, kind='reflection') is None
    hit = gather((0, 0), (2, 1), (2, 1), lambda p: (1000, 1000, 1000, 0) if p[0] else (2, 4, 6, .25), guide, kind='reflection')
    assert hit['count'] == 1 and hit['minimum'] == hit['maximum'] == (2, 4, 6)
    assert math.isclose(hit['mean'], 3.7192)
    for roughness in (math.nan, math.inf, 1.1):
        assert gather((0, 0), (1, 1), (1, 1), lambda _: (2, 4, 6, 1), lambda _: replace(Receiver(), roughness=roughness), kind='reflection') is None
    assert gather((0, 0), (1, 1), (1, 1), lambda _: (2, 4, 6, 1), guide,
                  kind='reflection', max_roughness=.4) is None
    negative = gather((0, 0), (1, 1), (1, 1), lambda _: (-2, -4, -6, 1), guide, kind='gi')
    assert negative['mean'] == 0 and negative['minimum'] == (-2, -4, -6)
    # FP16's finite HDR maximum has finite float32 first/second accumulation.
    hdr = gather((1, 1), (3, 3), (3, 3), lambda _: (65504, 65504, 65504, 1), guide, kind='gi')
    assert all(math.isfinite(struct.unpack('f', struct.pack('f', hdr[key]))[0]) for key in ('mean', 'second', 'variance'))
    assert hdr['variance'] >= 0 and math.isclose(hdr['mean'], 65504)
    outlier = gather((1, 1), (3, 3), (3, 3), lambda p: (100, 100, 100, 1) if p == (1, 1) else (0, 0, 0, 1), guide, kind='gi')
    assert outlier['count'] == 9 and math.isclose(outlier['mean'], 100/9) and outlier['variance'] > 0
    assert gather((0, 0), (1, 1), (1, 1), lambda _: (1,), guide, center_token=0) is None
    # Orthographic identity inverse VP, RH camera at Z=1: actual receiver-center
    # reconstruction gives a five-texel cross at metric tolerance .8.
    plane = lambda p: replace(Receiver(), position=(2*(p[0]+.5)/3-1, 1-2*(p[1]+.5)/3, .5))
    cross = gather((1, 1), (3, 3), (3, 3), signal, plane, center_position=(0, 0, .5),
                   camera=(0, 0, 1), absolute=.8, relative=0)
    assert cross['count'] == 5 and math.isclose(cross['mean'], .5)
    isolated = gather((1, 1), (3, 3), (3, 3), signal, plane, center_position=(0, 0, .5),
                      camera=(0, 0, 1), absolute=.01, relative=0)
    assert isolated['count'] == 1 and not isolated['variance_valid']
    rotated = gather((0, 0), (2, 1), (2, 1), lambda _: (.5,),
                     lambda _: replace(Receiver(), encoded_normal=(1, .5, .5)), center_normal=(1, 0, 0))
    assert rotated['count'] == 2 and rotated['variance_valid']
    assert gather((0, 0), (1, 1), (1, 1), lambda _: (2, 4, 6, 1e-7), guide, kind='reflection') is None
    confident = gather((0, 0), (2, 1), (2, 1), lambda _: (2, 4, 6, 1e-5), guide, kind='reflection')
    assert confident['count'] == 2 and confident['variance_valid']



def check_sources():
    canonical = []
    for language in ('glsl', 'metal'):
        path = ROOT / f'shaders/src/draw/lib/temporal_current_statistics.{language}'
        source = path.read_text()
        assert source.startswith('/*') and 'GNU General Public License' in source
        assert source.count('{') == source.count('}') and source.count('(') == source.count(')')
        assert 'CURRENT_STATISTICS_EPSILON = 1e-6' in source
        assert 'index < 9' in source and 'sampleCount >= 2u' in source
        assert 'max(statistics.secondMoment-statistics.mean*statistics.mean,0.0)' in source
        for forbidden in ('previousImage', 'outputImage', 'expectedPositionImage', 'imageStore', '.write(', 'oldWeight', 'reactivity', 'historyClipSigma'):
            assert forbidden not in source
        assert source.index('if (neighborToken == 0u') < source.index('float neighborDepth =')
        assert source.index('if (!finite4(sampleValue))') < source.index('sum += statistic;')
        assert source.index('if (') < source.index('uint neighborToken =')
        assert 'statistic = max(dot(color,' in source and 'clamp(sampleValue.r,0.0,1.0)' in source
        assert 'clamp(sampleValue.a,0.0,1.0) <= CURRENT_STATISTICS_EPSILON' in source
        block = source[source.index('    CurrentStatistics statistics;'):]
        block = re.sub(r'texelFetch\((?:u?sampler2D)\((\w+),nearestSampler\),(\w+),0\)', r'\1.read(\2)', block)
        block = re.sub(r'uint2\((\w+)\)', r'\1', block)
        block = re.sub(r'(positionImage|metadataDepthImage)\.read\(receiver\)\.r', r'\1.read(receiver)', block)
        block = block.replace('any(lessThan(neighbor,ivec2(0)))', 'any(neighbor < ivec2(0))')
        block = block.replace('any(greaterThanEqual(neighbor,ivec2(dataBlock.extent.zw)))', 'any(neighbor >= ivec2(dataBlock.extent.zw))')
        for a, b in (('ivec2', 'int2'), ('vec2', 'float2'), ('vec3', 'float3'), ('vec4', 'float4')):
            block = block.replace(a, b)
        canonical.append(block)
        shared = (ROOT / f'shaders/src/draw/lib/temporal_reprojection.{language}').read_text()
        call = shared.index('CurrentStatistics currentStatistics =')
        assert shared.rfind('if (dataBlock.adaptive.w > 0.5 && !frozen)', 0, call) > 0
        assert f'#if TEMPORAL_ADAPTIVE\n#include "temporal_current_statistics.{language}"\n#include "temporal_history_response.{language}"\n#endif' in shared
        assert 'float reactivity = response.reactivity;' in shared
    assert canonical[0] == canonical[1]


if __name__ == '__main__':
    check_math()
    check_sources()
    print('PASS: P4.2 neighborhood reference and paired source contracts. Shader/native execution: not_run.')
