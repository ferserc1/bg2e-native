#!/usr/bin/env python3
"""P4.4 bounded-moment reference and versioned source audit, no native execution."""
from pathlib import Path
import math
import re
import struct
import sys

sys.dont_write_bytecode = True
from feature_source import without_features
ROOT = Path(__file__).resolve().parents[3]


def half(x):
    return struct.unpack('e', struct.pack('e', x))[0]


def bounded(luminance, scalar=False):
    if not math.isfinite(luminance):
        raise ValueError('Reject nonfinite signal before moment arithmetic')
    return min(1, max(0, luminance)) if scalar else min(16, max(0, math.log2(1+max(luminance, 0))))


def valid(pair, scalar=False):
    return all(math.isfinite(x) and 0 <= x <= limit for x, limit in zip(pair, (1, 1) if scalar else (16, 256)))


def spatial(values, scalar=False):
    transformed = [bounded(v, scalar) for v in values]
    mean = math.fsum(transformed)/len(transformed)
    second = math.fsum(v*v for v in transformed)/len(transformed)
    return mean, max(second-mean*mean, 0)


def reactivity(previous, mean, variance, count, sigma=3, enough=True):
    if not enough:
        return 0
    if count >= 4:
        variance = max(variance, max(previous[1]-previous[0]*previous[0], 0))
    sigma_std = sigma*math.sqrt(variance+1e-6)
    delta = abs(previous[0]-mean)
    return min(1, max(0, (delta-sigma_std)/(delta+sigma_std+1e-6)))


def transport(taps, scalar=False):
    # A tap is (accepted-color, raw-weight, color, pair). Invalid pairs reject
    # the joint candidate so moment/color weights cannot select different surfaces.
    valid_taps = [t for t in taps if t[0] and t[1] > 0 and valid(t[3], scalar)]
    weight = math.fsum(t[1] for t in valid_taps)
    if weight <= 1e-6:
        return None
    color = math.fsum(t[1]*t[2] for t in valid_taps)/weight
    pair = tuple(math.fsum(t[1]*t[3][i] for t in valid_taps)/weight for i in range(2))
    return color, pair, weight


def update(current, previous, alpha, scalar=False, covered=True, accepted=True):
    if not covered:
        return (0, 0)
    statistic = bounded(current, scalar)
    pair = (statistic, statistic*statistic)
    return tuple(alpha*a+(1-alpha)*b for a, b in zip(pair, previous)) if accepted else pair


def check_math():
    for scalar in (False, True):
        for value in (0, 1e-6, .25, 1, 3, 1000, 65504, 1e30):
            s = bounded(value, scalar)
            assert valid((half(s), half(s*s)), scalar)
        for value in (math.nan, math.inf, -math.inf):
            try:
                bounded(value, scalar)
                raise AssertionError('nonfinite input accepted')
            except ValueError:
                pass
    assert bounded(65535) == 16 and bounded(1e30) == 16
    assert bounded(3) == 2 and bounded(-1) == 0 and bounded(2, True) == 1
    assert spatial((0, 3)) == (1, 1)
    assert not math.isclose(spatial((0, 3))[0], bounded(1.5))  # Mean(log), not log(mean).
    for count in (1, 2, 3):
        assert reactivity((1, 5), 2, 0, count) > .99  # Early confidence ignores temporal variance.
    assert reactivity((1, 5), 2, 0, 4) == 0
    assert reactivity((1, 0), 2, 0, 4) > .99  # Negative half-quantized variance floors to zero.
    assert reactivity((1, 5), 2, 0, 4, enough=False) == 0
    taps = [(True, .25, value, (value, value*value)) for value in (0, .25, .5, 1)]
    for mask in range(16):
        selected = [(bool(mask & (1 << i)), *t[1:]) for i, t in enumerate(taps)]
        result = transport(selected, True)
        count = mask.bit_count()
        if not count:
            assert result is None
        else:
            values = [t[2] for i, t in enumerate(taps) if mask & (1 << i)]
            assert math.isclose(result[0], sum(values)/count)
            assert result[1] == (result[0], sum(v*v for v in values)/count)
            assert result[2] == count/4
    for bad in ((math.nan, 0), (0, math.inf), (-1, 0), (17, 0), (0, 257)):
        mixed = taps[:3] + [(True, .25, 1000, bad)]
        result = transport(mixed)
        assert result[2] == .75 and result[0] < 1
    assert update(.75, (.25, .0625), 0, True) == (.25, .0625)  # Frozen transport.
    assert update(.75, (.25, .0625), .25, True) == (.375, .1875)
    assert update(.75, (.25, .0625), .25, True, accepted=False) == (.75, .5625)
    assert update(.75, (.25, .0625), .25, True, covered=False) == (0, 0)
    # HDR log and scalar moments remain separate per-key statistics; clipping is linear.
    assert update(3, (0, 0), .5) == (1, 2)
    assert update(1, (0, 0), .5, True) == (.5, .5)
    assert 4*1920*1080 == 8_294_400 and 4*3840*2160 == 33_177_600
    # Balanced branch stripping handles moments nested within adaptive weighting.
    fixture = '#if A\na\n#if B\nb\n#else\nc\n#endif\nd\n#endif\nz\n'
    assert without_features(fixture, {'A'}) == 'z\n'
    assert without_features(fixture, {'B'}) == '#if A\na\nc\nd\n#endif\nz\n'


def read(path):
    return (ROOT / path).read_text()


def check_sources():
    cpp = read('lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp')
    header = read('lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp')
    assert 'momentShaderInterface' in header and 'momentsStatus() const' in header
    assert 'const bool moments = adaptive && momentsAvailable()' in cpp
    assert 'moments ? 4u' in cpp and 'neighborhood-only v4 fallback' in cpp
    assert 'R16G16_SFLOAT).supports(momentUsage)' in cpp
    assert cpp.index('supports(momentUsage)') < cpp.index('_zeroMoments = device.createImage')
    for binding in ('{19,17}', '{20,18}'):
        assert binding in cpp
    assert 'next->moments = _pool->image(input.command,description)' in cpp
    assert 'previous ? previous->moments.get() : _zeroMoments.get()' in cpp
    assert 'moments && previous && !previous->moments' in cpp
    assert 'moments ? (diagnose ? _momentAcceptanceLayout' in cpp
    assert 'moments ? (diagnose ? _momentAcceptancePipelines' in cpp
    assert 'std::shared_ptr<gpu::Image> signal, position, normal, moments' in header
    assert cpp.index('invocation.previous = previous') < cpp.index('setSampledImage({19,17}')
    assert cpp.index('slot.invocations.push_back(std::move(invocation))') < cpp.index('input.command.dispatch(')
    assert 'sizeof(TemporalAdaptiveUniforms) == 368' in cpp
    moment_blocks = []
    transport_blocks = []
    owned = ['lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp', 'lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp',
             'lib/src/bg2e/draw/pipeline/EffectSettings.cpp', 'lib/src/bg2e/draw/RendererDeferred.cpp',
             'tests/draw/temporal/footprint_probe.hpp']
    for language in ('glsl', 'metal'):
        shared = read(f'shaders/src/draw/lib/temporal_reprojection.{language}')
        assert 'candidateValid = validMoments(candidateMoments)' in shared
        assert shared.index('candidateValid = validMoments(candidateMoments)') < shared.index('if (!candidateValid)')
        assert 'momentSum += candidateMoments*weight' in shared
        assert 'previousMoments = momentSum/validWeight' in shared
        assert 'resolvedMoments = mix(currentMoments,previousMoments,oldWeight)' in shared
        assert 'reactivity = momentReactivity(currentStatistics,previousMoments,oldCount,dataBlock.adaptive.z)' in shared
        # Compare every complete moment-only source region, including nested scalar branches.
        lines = shared.splitlines(keepends=True)
        regions = []
        for index, line in enumerate(lines):
            if line.strip() != '#if TEMPORAL_MOMENTS':
                continue
            depth, end = 1, index+1
            while depth:
                directive = lines[end].strip()
                if directive.startswith(('#if ', '#ifdef ', '#ifndef ')):
                    depth += 1
                elif directive == '#endif':
                    depth -= 1
                end += 1
            block = ''.join(lines[index+1:end-1])
            if 'binding=' in block or '[[texture(17)]]' in block:
                continue  # Resource declaration ABI is checked explicitly below.
            block = re.sub(r'texelFetch\(sampler2D\((\w+),nearestSampler\),(\w+),0\)', r'\1.read(\2)', block)
            block = re.sub(r'uint2\((\w+)\)', r'\1', block)
            block = block.replace('imageStore(outputMoments,ivec2(pixel),vec4(resolvedMoments,0,1));', 'STORE(resolvedMoments);')
            block = block.replace('outputMoments.write(float4(resolvedMoments,0,1),pixel);', 'STORE(resolvedMoments);')
            block = block.replace('vec2', 'float2').replace('vec3', 'float3').replace('vec4', 'float4')
            block = block.replace('.'+language+'"', '.LANG"')
            regions.append(block)
        transport_blocks.append(regions)
        if language == 'glsl':
            assert 'binding=19' in shared and 'binding=20,rg16f' in shared
        else:
            assert 'previousMomentImage [[texture(17)]]' in shared and 'outputMoments [[texture(18)]]' in shared
        for helper in ('temporal_moments', 'temporal_moment_reactivity'):
            path = f'shaders/src/draw/lib/{helper}.{language}'
            owned.append(path)
            source = read(path).replace('vec2', 'float2').replace('vec4', 'float4')
            if helper == 'temporal_moment_reactivity':
                assert 'effectiveCount >= 4.0' in source
                assert 'statistics.momentVariance' in source and 'statistics.variance;' not in source
                assert 'max(variance,temporalVariance)' in source
                moment_blocks.append(source)
        for diagnostic in (False, True):
            family = 'temporal_accumulation_v5'+('_diagnostic' if diagnostic else '')
            for profile in ('scalar', 'hdr'):
                path = f'shaders/src/draw/{family}/camera_{profile}.comp.{language}'
                owned.append(path)
                assert '#define TEMPORAL_MOMENTS 1' in read(path) and '#define TEMPORAL_ADAPTIVE 1' in read(path)
    assert moment_blocks[0] == moment_blocks[1]
    assert transport_blocks[0] == transport_blocks[1]
    for name in owned:
        source = read(name)
        assert source.startswith('/*') and 'GNU General Public License' in source
        assert source.count('{') == source.count('}') and source.count('(') == source.count(')'), name
        for include in re.findall(r'#include "([^\"]+)"', source):
            assert (ROOT / name).parent.joinpath(include).exists(), (name, include)


if __name__ == '__main__':
    check_math()
    check_sources()
    print('PASS: P4.4 bounded moments, transport, domain variance and versioned ownership source checks. Native: not_run.')
