#!/usr/bin/env python3
"""Validated footprint contract v1 oracle; does not execute production shaders."""
from dataclasses import dataclass, replace
import math
import re
from pathlib import Path
import sys
sys.dont_write_bytecode = True
from feature_source import without_features

CONTRACT = "draw.temporal.footprint.v1"
EPSILON = 1e-6


@dataclass(frozen=True)
class Tap:
    pixel: tuple
    metadata_pixel: tuple
    weight: float


@dataclass(frozen=True)
class Sample:
    signal: tuple = (0.0,)
    position: tuple = (0.0, 0.0, -2.0, 1.0)
    normal_count: tuple = (0.0, 0.0, 1.0, 4.0)
    token: int = 7


def footprint(uv, extent, metadata_extent, clip_valid=True):
    """Return positive-weight, in-bounds taps only; never clamp a candidate."""
    if any(e <= 0 for e in (*extent, *metadata_extent)):
        raise ValueError("Extents must be positive")
    if not clip_valid or not all(math.isfinite(v) and 0 <= v < 1 for v in uv):
        return ()
    q = tuple(u * e - 0.5 for u, e in zip(uv, extent))
    base = tuple(math.floor(v) for v in q)
    fraction = tuple(v - b for v, b in zip(q, base))
    taps = []
    for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
        pixel = (base[0] + dx, base[1] + dy)
        weight = (fraction[0] if dx else 1 - fraction[0]) * (fraction[1] if dy else 1 - fraction[1])
        if weight <= 0 or not all(0 <= p < e for p, e in zip(pixel, extent)):
            continue
        # Integer form of floor(((pixel + .5) / extent) * metadata_extent).
        metadata = tuple((2 * p + 1) * m // (2 * e)
                         for p, e, m in zip(pixel, extent, metadata_extent))
        taps.append(Tap(pixel, metadata, weight))
    return tuple(taps)


IDENTITY = ((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))


def view_point(view, point):
    return tuple(sum(a * b for a, b in zip(row, (*point, 1))) for row in view)


def depth_valid(candidate, expected, view, tolerance):
    a, b = view_point(view, candidate), view_point(view, expected)
    return (all(math.isfinite(v) for v in (*a, *b, tolerance)) and tolerance >= 0
            and -a[2] > 0 and -b[2] > 0 and abs(a[2] - b[2]) <= tolerance)


def validated(taps, fetch, token=7, expected=(0.0, 0.0, -2.0),
              direction=(0.0, 0.0, 1.0), tolerance=0.02, cosine=0.9, previous_view=IDENTITY):
    """Return raw support W and normalized accepted taps, without signal reduction.

    fetch(tap) supplies that tap's signal/guides and candidate-center metadata token.
    Caller has already validated current coverage/metadata and expected geometry.
    Both previous-view depths must be finite, positive and within tolerance.
    """
    accepted = []
    for tap in taps:
        sample = fetch(tap)
        if not all(math.isfinite(v) for v in (*sample.signal, *sample.position, *sample.normal_count)):
            continue
        if token == 0 or sample.token == 0 or sample.token != token:
            continue
        if sample.position[3] <= 0.5 or sample.normal_count[3] < 1:
            continue
        if not depth_valid(sample.position[:3], expected, previous_view, tolerance):
            continue
        if math.dist(sample.position[:3], expected) > tolerance:
            continue
        if sum(a * b for a, b in zip(sample.normal_count[:3], direction)) < cosine:
            continue
        accepted.append((tap, sample))
    support = sum(tap.weight for tap, _ in accepted)
    if support <= EPSILON:
        return support, ()
    return support, tuple((tap, sample, tap.weight / support) for tap, sample in accepted)


def reduce_history(support, accepted, kind, maximum):
    """Independent normalized spatial oracle: signal, minimum count, support."""
    if not accepted:
        return None
    if kind == 'gi':
        # Signal validity participates in support, before normalization.
        kept = [(tap, sample) for tap, sample, _ in accepted if sample.signal[3] > .5]
        support = sum(tap.weight for tap, _ in kept)
        if support <= EPSILON:
            return None
        accepted = [(tap, sample, tap.weight / support) for tap, sample in kept]
    count = min(maximum, *(sample.normal_count[3] for _, sample, _ in accepted))
    if kind == 'reflection':
        confidence = sum(weight * min(1, max(0, sample.signal[3])) for _, sample, weight in accepted)
        rgb = tuple(sum(weight * min(1, max(0, sample.signal[3])) * sample.signal[c]
                        for _, sample, weight in accepted) / confidence if confidence > 1e-8 else 0
                    for c in range(3))
        signal = (*rgb, confidence)
    else:
        signal = tuple(sum(weight * sample.signal[c] for _, sample, weight in accepted)
                       for c in range(1 if kind == 'scalar' else 3))
        if kind == 'gi':
            signal = (*signal, 1)
    return signal, count, min(1, max(0, support))


def accumulate(current, history, maximum, freeze=False, kind='scalar'):
    if history is None:
        return current, 1
    previous, old_count, _ = history
    weight = 1 if freeze and old_count >= maximum else 1 - 1 / min(old_count + 1, maximum)
    if kind == 'reflection':
        confidence = current[3] * (1 - weight) + previous[3] * weight
        signal = tuple((current[c] * current[3] * (1 - weight) + previous[c] * previous[3] * weight)
                       / confidence if confidence > 1e-8 else 0 for c in range(3)) + (confidence,)
    else:
        signal = tuple(a * (1 - weight) + b * weight for a, b in zip(current, previous))
    return signal, min(old_count + 1, maximum)


def check_reduction(taps):
    def reduce(samples, kind='scalar', maximum=32):
        support, accepted = validated(taps, lambda t: samples[taps.index(t)])
        return reduce_history(support, accepted, kind, maximum)
    samples = [replace(Sample(), signal=(value,), normal_count=(0, 0, 1, count))
               for value, count in zip((0, 1, .5, .5), (32, 1, 20, 10))]
    h = reduce(samples)
    assert h == ((.5,), 1, 1)
    assert accumulate((1,), h, 32) == ((.75,), 2)
    # Invalid bright foreground contributes neither color nor long count.
    samples[1] = replace(samples[1], token=9, signal=(1000,))
    h = reduce(samples)
    assert math.isclose(h[0][0], 1/3) and h[1:] == (10, .75)
    assert reduce(samples, maximum=2)[1] == 2
    saturated = reduce([replace(Sample(), signal=(.25,), normal_count=(0, 0, 1, 32))] * 4)
    assert accumulate((.75,), saturated, 32, freeze=True) == ((.25,), 32)
    assert accumulate((.75,), saturated, 32, freeze=False)[0][0] > .25
    assert accumulate((.75,), None, 32, freeze=True) == ((.75,), 1)
    gi = [replace(Sample(), signal=(0, 0, 0, 1))] * 2 + [replace(Sample(), signal=(100, 100, 100, 0))] * 2
    assert reduce(gi, 'gi') == ((0, 0, 0, 1), 4, .5)
    assert reduce([replace(Sample(), signal=(0, 0, 0, 0))] * 4, 'gi') is None
    reflection = [replace(Sample(), signal=(8, 0, 0, .25)),
                  replace(Sample(), signal=(0, 4, 0, .75)),
                  replace(Sample(), signal=(1000, 1000, 1000, 0)),
                  replace(Sample(), signal=(1000, 1000, 1000, 0))]
    h = reduce(reflection, 'reflection')
    assert h == ((2, 3, 0, .25), 4, 1)
    result, count = accumulate((0, 0, 4, .5), h, 32, kind='reflection')
    assert all(math.isclose(a, b) for a, b in zip(result, (4/3, 2, 4/3, .3))) and count == 5
    assert reduce([replace(Sample(), signal=(50, 50, 50, 0))] * 4, 'reflection') == ((0, 0, 0, 0), 4, 1)
    # Scalar FP16 readback values remain bounded after normalized valid weighting.
    import struct
    half = lambda x: struct.unpack('e', struct.pack('e', x))[0]
    assert abs(half(reduce(samples)[0][0]) - 1/3) < .001


REASON_COLORS = ((.25, .25, .25), (.5, 0, 1), (0, .25, 1), (0, 0, 0),
                 (1, 0, 1), (1, 0, 0), (1, 1, 0), (1, 1, 1), (0, 1, 1), (0, 1, 0))


def diagnostic_reason(failures, support=0):
    # Accepted support dominates failures of rejected neighbors. No counters/readback.
    if support > EPSILON:
        return 9 if support >= 1 - EPSILON else 8
    return min(failures, default=7)


def sample_failure(sample, expected=(0, 0, -2), direction=(0, 0, 1)):
    failures = []
    if sample.position[3] <= .5 or not math.isfinite(sample.normal_count[3]) or sample.normal_count[3] < 1:
        failures.append(3)
    if sample.token != 7:
        failures.append(4)
    if (not all(math.isfinite(x) for x in sample.position) or
            not depth_valid(sample.position[:3], expected, IDENTITY, .02) or
            math.dist(sample.position[:3], expected) > .02):
        failures.append(5)
    if (not all(math.isfinite(x) for x in sample.normal_count) or
            sum(a*b for a, b in zip(direction, sample.normal_count[:3])) < .9):
        failures.append(6)
    if not all(math.isfinite(x) for x in sample.signal):
        failures.append(7)
    return min(failures, default=None)


def check_diagnostics(taps):
    assert len(set(REASON_COLORS)) == 10
    assert sample_failure(Sample()) is None
    cases = ((replace(Sample(), token=8), 4),
             (replace(Sample(), token=8, position=(0, 0, -2, 0)), 3),
             (replace(Sample(), position=(0, 0, -3, 1), normal_count=(1, 0, 0, 4)), 5),
             (replace(Sample(), normal_count=(1, 0, 0, 4), signal=(math.nan,)), 6),
             (replace(Sample(), signal=(math.inf,)), 7))
    for sample, reason in cases:
        assert sample_failure(sample) == reason
    for reason in range(8):
        assert diagnostic_reason([reason, 7]) == reason
        assert diagnostic_reason([7, reason]) == reason
    assert diagnostic_reason(range(8)) == 0
    for mask in range(16):
        support, accepted = validated(taps, lambda t: Sample(token=7 if mask & (1 << taps.index(t)) else 8))
        assert diagnostic_reason([4], support) == (4 if not mask else (9 if mask == 15 else 8))
    # Alpha-cutout/blended-edge-like coverage: uncovered guides cannot leak bright values.
    edge = [replace(Sample(), signal=(1000,), position=(0, 0, -2, 0)), Sample(), Sample(), Sample()]
    support, accepted = validated(taps, lambda t: edge[taps.index(t)])
    assert support == .75 and reduce_history(support, accepted, 'scalar', 32)[0] == (0,)
    assert diagnostic_reason([3], support) == 8
    assert diagnostic_reason([5, 6, 7]) == 5
    assert diagnostic_reason([4, 3, 5]) == 3
    assert diagnostic_reason([7], 1e-7) == 7
    # Rotated object normal is accepted against its expected previous direction.
    rotated = replace(Sample(), normal_count=(1, 0, 0, 4))
    assert len(validated(taps, lambda _: rotated, direction=(1, 0, 0))[1]) == 4
    assert not validated(taps, lambda _: rotated)[1]
    # Distinct token at identical geometry, and same token across depth discontinuity.
    assert not validated(taps, lambda _: replace(Sample(), token=8))[1]
    assert not validated(taps, lambda _: replace(Sample(), position=(0, 0, -1, 1)))[1]


def check():
    taps = footprint((0.5, 0.5), (2, 2), (5, 3))
    assert [t.pixel for t in taps] == [(0, 0), (1, 0), (0, 1), (1, 1)]
    assert [t.metadata_pixel for t in taps] == [(1, 0), (3, 0), (1, 2), (3, 2)]
    assert all(t.weight == 0.25 for t in taps)
    # Exact center, fractional footprint, odd extent, lower-resolution metadata.
    assert footprint((0.5, 0.5), (3, 3), (2, 2)) == (Tap((1, 1), (1, 1), 1.0),)
    fractional = footprint((0.375, 0.625), (2, 2), (2, 2))
    assert [t.weight for t in fractional] == [0.1875, 0.0625, 0.5625, 0.1875]
    for uv in ((0, 0), (0, 0.999), (0.999, 0), (0.999, 0.999)):
        edge = footprint(uv, (1, 1), (3, 5))
        assert len(edge) == 1 and edge[0].metadata_pixel == (1, 2)
        support, accepted = validated(edge, lambda _: Sample())
        assert 0 < support < 1 and accepted[0][2] == 1
    for uv in ((1, 0.5), (-0.001, 0.5), (math.nan, 0.5), (math.inf, 0.5)):
        assert not footprint(uv, (2, 2), (3, 3))
    assert not footprint((0.5, 0.5), (2, 2), (3, 3), clip_valid=False)
    # Every corner can be the sole survivor; also test two/four/no survivors.
    for mask in range(16):
        def fetch(tap):
            index = taps.index(tap)
            return Sample(signal=(float(index),), token=7 if mask & (1 << index) else 0)
        support, accepted = validated(taps, fetch)
        assert support == mask.bit_count() / 4
        assert len(accepted) == mask.bit_count()
        if accepted:
            assert math.isclose(sum(weight for _, _, weight in accepted), 1)
    bad = [replace(Sample(), token=0), replace(Sample(), token=8),
           replace(Sample(), position=(0, 0, -2, 0)),
           replace(Sample(), position=(0, 0, -3, 1)),
           replace(Sample(), normal_count=(0, 0, -1, 4)),
           replace(Sample(), normal_count=(0, 0, 1, 0))]
    for field in ('signal', 'position', 'normal_count'):
        for nonfinite in (math.nan, math.inf, -math.inf):
            values = list(getattr(Sample(), field))
            values[0] = nonfinite
            bad.append(replace(Sample(), **{field: tuple(values)}))
    for sample in bad:
        assert validated(taps, lambda _: sample)[1] == ()
    assert not validated(taps, lambda _: Sample(), token=0)[1]
    # Very small surviving support rejects; zero-weight neighbors cannot poison it.
    tiny = footprint((0.2500001, 0.25), (2, 2), (2, 2))
    assert not validated(tiny, lambda t: Sample(token=7 if t.pixel == (1, 0) else 0))[1]
    center = footprint((0.25, 0.25), (2, 2), (2, 2))
    assert len(center) == 1 and validated(center, lambda _: Sample())[0] == 1
    # Black is a valid signal. Spatial signal/count/confidence reduction is P3.3.
    assert len(validated(taps, lambda _: Sample())[1]) == 4
    check_reduction(taps)
    check_diagnostics(taps)
    # Paired source contract and preservation of temporal blend/output semantics.
    root = Path(__file__).resolve().parents[3]
    predicates = []
    footprints = []
    diagnostic_helpers = []
    for lang in ('glsl', 'metal'):
        source = without_features((root / f'shaders/src/draw/lib/temporal_reprojection.{lang}').read_text(),
                                  {'TEMPORAL_ADAPTIVE', 'TEMPORAL_MOMENTS', 'TEMPORAL_METRICS'})
        diagnostic_helpers.append(source[source.index('reasonColor('):source.index('\n#endif', source.index('reasonColor('))].replace('vec3', 'float3'))
        assert 'uint reason = accepted ? (geometricConfidence >= 1.0-1e-6 ? 9u : 8u) : rejectionReason;' in source
        start = source.index('    float expectedDepth =')
        end = source.index('\n}', start)
        predicate = source[start:end]
        predicates.append(predicate.replace('!isnan(threshold) && !isinf(threshold)', 'isfinite(threshold)'))
        for expression in ('expectedDepth > 0.0 && candidateDepth > 0.0',
                           'abs(candidateDepth-expectedDepth) <= threshold',
                           'length(previousPosition.xyz-expectedPosition) <= threshold',
                           'dot(expectedDirection,previousNormal.xyz) >= normalCosine'):
            assert expression in predicate
        assert 'bool candidateValid = finite4(candidate) && validPreviousGeometry(' in source
        sampling_source = re.sub(r'#if TEMPORAL_DIAGNOSTIC\n.*?#endif\n', '', source, flags=re.S)
        block = sampling_source[sampling_source.index('        '+('vec2' if lang == 'glsl' else 'float2')+' q ='):sampling_source.index('            float oldWeight')]
        block = re.sub(r'texelFetch\((?:u?sampler2D)\((\w+),nearestSampler\),(\w+),0\)', r'\1.read(\2)', block)
        block = re.sub(r'uint2\((\w+)\)', r'\1', block)
        block = block.replace('textureSize(usampler2D(previousTokenImage,nearestSampler),0)', 'METADATA_EXTENT')
        block = block.replace('int2(previousTokenImage.get_width(),previousTokenImage.get_height())', 'METADATA_EXTENT')
        block = block.replace('any(lessThan(previousPixel,ivec2(0)))', 'any(previousPixel < ivec2(0))')
        block = block.replace('any(greaterThanEqual(previousPixel,ivec2(dataBlock.extent.zw)))', 'any(previousPixel >= ivec2(dataBlock.extent.zw))')
        for a, b in (('ivec2', 'int2'), ('vec2', 'float2'), ('vec3', 'float3'), ('vec4', 'float4')):
            block = block.replace(a, b)
        footprints.append(block)
        original = (root / f'shaders/src/draw/lib/temporal.{lang}').read_text()
        assert source[source.index('            float oldWeight'):].split('\n#if TEMPORAL_DIAGNOSTIC')[0] == original[original.index('            float oldWeight'):].split('\n#if TEMPORAL_DIAGNOSTIC')[0]
        for expression in ('tap < 4', 'minimumCount = min(minimumCount,previousNormal.w)',
                           'validWeight > 1e-6', 'candidate.rgb *= candidate.a',
                           'geometricConfidence = accepted ? clamp(validWeight,0.0,1.0)',
                           'candidateUV*', 'previousToken != token'):
            assert expression in source
        assert source.index('if (!candidateValid)') < source.index('historySum += candidate*weight;')
    assert predicates[0] == predicates[1]
    assert footprints[0] == footprints[1]
    assert diagnostic_helpers[0] == diagnostic_helpers[1]
    # Depth uses the previous camera, including rotation/translation, not world Z.
    rotated = ((0, 0, 1, 3), (0, 1, 0, 0), (-1, 0, 0, -2), (0, 0, 0, 1))
    assert depth_valid((1, 0, 5), (1.01, 0, 5), rotated, 0.02)
    assert not depth_valid((-2, 0, -5), (-2, 0, -5), rotated, 0.02)
    for z in (0, 1):
        assert not depth_valid((0, 0, z), (0, 0, z), IDENTITY, 0.02)
    assert depth_valid((0, 0, -2.125), (0, 0, -2), IDENTITY, 0.125)
    assert not depth_valid((0, 0, -2.126), (0, 0, -2), IDENTITY, 0.125)
    assert not depth_valid((0, 0, -1.8), (0, 0, -2), IDENTITY, 0.125)
    for value in (math.nan, math.inf, -math.inf):
        assert not depth_valid((0, 0, value), (0, 0, -2), IDENTITY, 0.02)
        assert not depth_valid((0, 0, -2), (0, 0, value), IDENTITY, 0.02)
        assert not depth_valid((0, 0, -2), (0, 0, -2), IDENTITY, value)
    assert not depth_valid((0, 0, -2), (0, 0, -2), IDENTITY, -0.01)
    # Full world-distance remains mandatory even when view depths agree exactly.
    assert not validated(taps, lambda _: replace(Sample(), position=(1, 0, -2, 1)))[1]
    assert not validated(taps, lambda _: replace(Sample(), position=(0, 0, 0, 1)),
                         expected=(0, 0, 0))[1]
    print(f"{CONTRACT}: coordinate, candidate validation and normalization checks passed")


if __name__ == '__main__':
    check()
