#!/usr/bin/env python3
"""P4.1 source/independent math acceptance; never engine/shader execution."""
from pathlib import Path
import itertools
import math
import re
import sys
sys.dont_write_bytecode = True
from feature_source import without_features

ROOT = Path(__file__).resolve().parents[3]
FIELDS = ('adaptiveEnabled', 'minCurrentWeight', 'reactiveCurrentWeight', 'historyClipSigma')


def read(name):
    return (ROOT / name).read_text()


def valid(minimum, reactive, sigma):
    return all(math.isfinite(v) for v in (minimum, reactive, sigma)) and 0 <= minimum <= reactive <= 1 and 0 <= sigma <= 10


def alpha(count, maximum, support, enabled, minimum=.03125, reactive=.25, freeze=False, accepted=True):
    if not accepted:
        return 1
    n = min(count, maximum)
    if freeze and n >= maximum:
        return 0
    baseline = 1 / min(n + 1, maximum)
    return min(1, max(baseline, minimum, (1-support)*reactive, 0)) if enabled else baseline


def check_math():
    assert valid(.03125, .25, 3) and valid(0, 0, 0) and valid(1, 1, 10)
    for values in ((-.1, .25, 3), (.5, .25, 3), (0, 1.1, 3), (0, 1, -1), (0, 1, 10.1)):
        assert not valid(*values)
    for i, value in itertools.product(range(3), (math.nan, math.inf, -math.inf)):
        values = [.03125, .25, 3]
        values[i] = value
        assert not valid(*values)
    for maximum, count, support in itertools.product((1, 2, 32, 1024), (1, 2, 31, 1024), (.00001, .25, .5, 1)):
        base = alpha(count, maximum, support, False)
        adaptive = alpha(count, maximum, support, True)
        assert 0 <= base <= adaptive <= 1
        assert alpha(count, maximum, support, True, freeze=True, accepted=False) == 1
        if count >= maximum:
            assert alpha(count, maximum, support, True, freeze=True) == 0
        assert alpha(count, maximum, support, False, minimum=1, reactive=1) == base
    assert alpha(31, 32, .25, True) == .1875
    assert alpha(31, 32, 1, True) == .03125
    assert alpha(1, 32, .25, True, minimum=0, reactive=1) == .75
    assert alpha(31, 32, 1, True, minimum=.8, reactive=.9) == .8
    assert alpha(1024, 1024, .25, True, freeze=True) == 0
    assert alpha(1024, 1024, .25, True, freeze=False) == .1875
    # Valid proposed catalog tuples can stage through either reflection-property order.
    for start, final, order in itertools.product(((.03125, .25), (.8, .9), (0, 1)),
                                                ((0, 0), (.1, .2), (.8, .9), (1, 1)),
                                                ((0, 1), (1, 0))):
        state = list(start)
        for i in order:
            if i == 0:
                state = [final[0], max(final[0], state[1])]
            else:
                state = [min(final[1], state[0]), final[1]]
            assert valid(*state, 3)
        assert tuple(state) == final


def check_sources():
    header = read('lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp')
    assert 'bool adaptiveEnabled = false' in header
    assert 'minCurrentWeight = 0.03125f, reactiveCurrentWeight = 0.25f, historyClipSigma = 3.0f' in header
    accumulator = read('lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp')
    assert 'sizeof(TemporalAdaptiveUniforms) == 368' in accumulator
    assert 'offsetof(TemporalAdaptiveUniforms, adaptive) == 352' in accumulator
    for size in (208, 288, 352):
        assert f'== {size}' in accumulator
    assert 'rigid && _settings.adaptiveEnabled && adaptiveAvailable()' in accumulator
    assert 'adaptive ? 3u : (rigid ? 2u' in accumulator
    assert 'adaptive ? sizeof(adaptiveData)' in accumulator
    assert 'adaptive ? (diagnose ? _adaptiveAcceptanceLayout' in accumulator
    assert 'adaptive ? (diagnose ? _adaptiveAcceptancePipelines' in accumulator
    record = accumulator[accumulator.index('TemporalResult TemporalAccumulator::record('):accumulator.index('bool TemporalAccumulator::diagnosticsAvailable')]
    assert 'setAdaptiveSettings(' not in record
    setter = accumulator[accumulator.index('void TemporalAccumulator::setAdaptiveSettings'):accumulator.index('bool TemporalAccumulator::adaptiveAvailable')]
    assert setter.count('invalidate();') == 1
    assert setter.index('validateAdaptiveSettings(') < setter.index('invalidate();') < setter.index('_settings.adaptiveEnabled =')
    assert '!= value' not in setter  # Explicit complete tuple equality check, not a per-frame setter.
    for field in FIELDS:
        assert f'type.accessor<' in read('lib/src/bg2e/draw/pipeline/EffectSettings.cpp')
        assert f'"{field}"' in read('lib/src/bg2e/draw/pipeline/EffectSettings.cpp')
    # All three settings payload/equality sequences must append these in the same order.
    cached = read('lib/src/bg2e/draw/TemporalSignatureCache.cpp')
    reference = read('lib/src/bg2e/draw/RendererDeferred.cpp')
    for prefix, source in (('s.temporal.', cached), ('key->temporal.', cached), ('temporal.', reference[reference.index('append(temporal.enabled)'):])):
        indices = [source.index(prefix + field) for field in FIELDS]
        assert indices == sorted(indices) and len(set(indices)) == 4
    preferences = read('lib/src/bg2e/draw/RenderSettingsPreferences.cpp')
    assert 'pipeline::TemporalAccumulator::validateAdaptiveSettings(' in preferences
    assert 'configuration.adaptiveShaderInterface = "draw.temporal.v4"' in reference
    blocks = []
    owned = ['lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp',
             'lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp',
             'lib/src/bg2e/draw/pipeline/EffectSettings.cpp',
             'lib/src/bg2e/draw/RenderSettingsPreferences.cpp',
             'lib/src/bg2e/draw/TemporalSignatureCache.cpp',
             'lib/src/bg2e/draw/RendererDeferred.cpp',
             'tests/draw/settings_codec/probe.hpp', 'tests/draw/temporal/footprint_probe.hpp',
             'tests/draw/temporal_invalidation/probe.hpp', 'tests/draw/temporal/adaptive_probe.hpp']
    for language in ('glsl', 'metal'):
        path = f'shaders/src/draw/lib/temporal_reprojection.{language}'
        owned.append(path)
        source = without_features(read(path), {"TEMPORAL_MOMENTS", "TEMPORAL_METRICS"})
        block = re.search(r'#if TEMPORAL_ADAPTIVE\n(            // Freeze.*?)#endif', source, re.S).group(1)
        assert 'dataBlock.adaptive.w > 0.5 && !frozen' in block
        assert 'float reactivity = response.reactivity' in block and '(1.0-geometricConfidence)' in block
        assert 'respondToCurrentStatistics(previous,currentStatistics,dataBlock.adaptive.z)' in block
        assert 'oldCount = min(oldCount,trustCap)' in block
        assert 'oldWeight = 1.0-alpha' in block and 'count =' not in block
        # Metal passes texture/uniform handles explicitly; GLSL accesses globals.
        canonical = block.replace('int2(pixel)', 'ivec2(pixel)').replace(',\n                    dataBlock,normalImage,positionImage,currentImage,materialImage,tokenImage,metadataDepthImage', '')
        blocks.append(canonical)
        for family, diagnostic in (('temporal_accumulation_v4', False), ('temporal_accumulation_v4_diagnostic', True)):
            for profile in ('scalar', 'hdr'):
                wrapper = f'shaders/src/draw/{family}/camera_{profile}.comp.{language}'
                owned.append(wrapper)
                contents = read(wrapper)
                assert '#define TEMPORAL_ADAPTIVE 1' in contents
                assert '#define TEMPORAL_METADATA 1' in contents
                assert ('#define TEMPORAL_DIAGNOSTIC 1' in contents) == diagnostic
    assert blocks[0] == blocks[1]
    for name in owned:
        source = read(name)
        assert source.startswith('/*') and 'GNU General Public License' in source
        # Ignore strings/comments, including deliberately malformed JSON test strings.
        tokens = re.sub(r'R"\((.*?)\)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
        assert tokens.count('{') == tokens.count('}'), name
        assert tokens.count('(') == tokens.count(')'), name
        for include in re.findall(r'#include "([^\"]+)"', source):
            assert (ROOT / name).parent.joinpath(include).exists(), (name, include)
        for include in re.findall(r'#include <(bg2e/[^>]+)>', source):
            assert (ROOT / 'lib/include' / include).exists(), (name, include)


if __name__ == '__main__':
    check_math()
    check_sources()
    # Independent per-step references remain separate from native execution.
    import sys
    sys.dont_write_bytecode = True
    from neighborhood_reference import check_math as check_neighborhood_math, check_sources as check_neighborhood_sources
    check_neighborhood_math()
    check_neighborhood_sources()
    from history_response_reference import check_math as check_response_math, check_sources as check_response_sources
    check_response_math()
    check_response_sources()
    from moments_reference import check_math as check_moments_math, check_sources as check_moments_sources
    check_moments_math()
    check_moments_sources()
    from adaptive_sequence_reference import check_math as check_sequence_math, check_sources as check_sequence_sources
    check_sequence_math()
    check_sequence_sources()
    from diagnostic_reference import check_math as check_diagnostic_math, check_sources as check_diagnostic_sources
    check_diagnostic_math()
    check_diagnostic_sources()
    print('PASS: P4.1–P4.6 references/source/cost diagnostics. Compilation/native: not_run.')
