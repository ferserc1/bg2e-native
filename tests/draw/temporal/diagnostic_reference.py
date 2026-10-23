#!/usr/bin/env python3
"""P4.6 diagnostic encoding/cost/source audit, not compiler/native validation."""
from pathlib import Path
import math
import re
import struct
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]


def half(value):
    return struct.unpack('e', struct.pack('e', value))[0]


def encode(alpha, reaction, variance, count, maximum, covered=True):
    if not covered:
        return (0, 0, 0, 0)
    assert all(math.isfinite(v) for v in (alpha, reaction, variance, count)) and variance >= 0
    return tuple(half(min(1, max(0, v))) for v in (alpha, reaction, variance/(1+variance), count/maximum))


def display(values, mode, token=7, available=True):
    if not available:
        return (1, .5, 0)
    value = values[mode-6]
    if token == 0 or not math.isfinite(value):
        return (0, 0, 0)
    return (min(1, max(0, value)),)*3


def regions(source):
    lines, result = source.splitlines(keepends=True), []
    for start, line in enumerate(lines):
        if line.strip() != '#if TEMPORAL_METRICS':
            continue
        depth, end = 1, start+1
        while depth:
            text = lines[end].strip()
            if text.startswith(('#if ', '#ifdef ', '#ifndef ')):
                depth += 1
            elif text == '#endif':
                depth -= 1
            end += 1
        result.append(''.join(lines[start+1:end-1]))
    return result


def check_math():
    cold = encode(1, 0, 0, 1, 32)
    assert cold == (1, 0, 0, .03125)
    frozen = encode(0, 0, 0, 32, 32)
    assert frozen == (0, 0, 0, 1)
    assert encode(1, 0, 0, 0, 32, covered=False) == (0, 0, 0, 0)
    assert encode(.25, .5, 1, 2, 32) == (.25, .5, .5, .0625)
    for variance in (0, 1e-6, .25, 1, 16, 256, 4e9):
        values = encode(.25, .5, variance, 16/3, 32)
        assert all(math.isfinite(v) and 0 <= v <= 1 for v in values)
        for view in range(6, 10):
            assert display(values, view) == (values[view-6],)*3
            assert display(values, view, available=False) == (1, .5, 0)
            assert display(values, view, token=0) == (0, 0, 0)
    assert display((math.nan, 0, 0, 0), 6) == (0, 0, 0)
    # Variance channels are visual encodings, not interchangeable physical units.
    assert encode(1, 0, 4e9, 1, 32)[2] == 1
    assert encode(1, 0, 1, 1, 32)[2] == .5
    # One requested production capture/display; retention and other domains are extra.
    pixels = 1920*1080
    assert 8*pixels+8*pixels == 33_177_600
    assert 4*pixels == 8_294_400
    assert (26+4+8)*pixels == 78_796_800  # Scalar v5 history plus active capture, one generation.
    assert ((30+36+36)+36)*pixels == 286_156_800
    assert ((30+36+36)+36+8+8)*pixels == 319_334_400
    assert (32+4+8)*pixels == 91_238_400  # HDR equivalent; receiver display/metadata excluded.


def check_sources():
    header = (ROOT / 'lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp').read_text()
    cpp = (ROOT / 'lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp').read_text()
    assert 'CurrentWeight, Reactivity, Variance, EffectiveCount' in header
    enum_body = header[header.index('enum class TemporalDiagnosticView'):].split('{',1)[1].split('}',1)[0]
    assert [name.strip() for name in enum_body.split(',')] == ['Off','Tokens','LocalHistory','Motion',
        'PreviousPosition','PreviousNormal','CurrentWeight','Reactivity','Variance','EffectiveCount']
    assert 'uint32_t(view) <= uint32_t(TemporalDiagnosticView::PreviousNormal)' in cpp
    assert 'uint32_t(TemporalDiagnosticView::EffectiveCount)' in cpp
    assert 'bool diagnoseAdaptive = false' in header
    assert 'input.diagnoseAcceptance && input.diagnoseAdaptive' in cpp
    assert 'const bool metrics = adaptive && input.diagnoseAdaptive' in cpp
    assert 'if (metrics)\n    {\n        next->metrics = ' in cpp
    assert 'setStorageImage({21,19},next->metrics.get())' in cpp
    assert 'metrics ? _metricLayouts[moments ? 1u : 0u]' in cpp
    assert 'metrics ? _metricPipelines[metricIndex].get()' in cpp
    assert 'metricView ? _metricDisplayLayout : _diagnosticLayout' in cpp
    assert 'found->second->recording == command.recordingCompletion()' in cpp
    assert 'metricView ? bool(found->second->metrics)' in cpp
    assert 'Optional adaptive metrics unavailable' in cpp
    owned = ['lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp', 'lib/include/bg2e/draw/RendererDeferred.hpp',
             'lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp', 'lib/src/bg2e/draw/pipeline/EffectSettings.cpp',
             'lib/src/bg2e/draw/RendererDeferred.cpp', 'lib/src/bg2e/draw/RenderSettingsPreferences.cpp',
             'tests/draw/temporal/adaptive_probe.hpp']
    captures = []
    for lang in ('glsl', 'metal'):
        source = (ROOT / f'shaders/src/draw/lib/temporal_reprojection.{lang}').read_text()
        owned.append(f'shaders/src/draw/lib/temporal_reprojection.{lang}')
        blocks = []
        for block in regions(source):
            assert not re.search(r'(?<![\w.])(?:oldCount|oldWeight|previous|count|current|result|accepted|validWeight|resolvedMoments|reactivity)\s*=(?!=)', block)
            if 'binding=21' in block or '[[texture(19)]]' in block:
                continue
            block = block.replace('vec4', 'float4')
            block = block.replace('imageStore(outputMetrics,ivec2(pixel),metrics);', 'STORE(metrics);')
            block = block.replace('outputMetrics.write(metrics,pixel);', 'STORE(metrics);')
            blocks.append(block)
        captures.append(blocks)
        for expression in ('diagnosticAlpha = 1.0-oldWeight', 'diagnosticReactivity = reactivity',
                           'diagnosticVariance = currentStatistics.momentVariance',
                           'diagnosticVariance = currentStatistics.variance', 'count/float(dataBlock.counts.y)'):
            assert expression in source
        for version in (4, 5):
            for profile in ('scalar', 'hdr'):
                name = f'shaders/src/draw/temporal_accumulation_v{version}_metrics/camera_{profile}.comp.{lang}'
                owned.append(name)
                contents = (ROOT / name).read_text()
                assert '#define TEMPORAL_METRICS 1' in contents and '#define TEMPORAL_ADAPTIVE 1' in contents
                assert '#define TEMPORAL_DIAGNOSTIC 1' not in contents
                assert ('#define TEMPORAL_MOMENTS 1' in contents) == (version == 5)
        name = f'shaders/src/draw/temporal_diagnostic/metrics.comp.{lang}'
        owned.append(name)
        display_source = (ROOT / name).read_text()
        assert 'dataBlock.options.w != 0u' in display_source and 'dataBlock.options.z-6u' in display_source
        assert 'token != 0u' in display_source
    assert captures[0] == captures[1]
    renderer = (ROOT / 'lib/src/bg2e/draw/RendererDeferred.cpp').read_text()
    assert 'slot.diagnosticSignal == kind && slot.diagnosticLayer == layer' in renderer
    assert 'temporalAdaptiveDiagnosticsAvailable' in renderer
    for name in owned:
        source = (ROOT / name).read_text()
        assert source.startswith('/*') and 'GNU General Public License' in source
        code = re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
        assert code.count('{') == code.count('}') and code.count('(') == code.count(')'), name
        for include in re.findall(r'#include "([^\"]+)"', source):
            assert (ROOT / name).parent.joinpath(include).exists()
    probe = (ROOT / 'tests/draw/temporal/adaptive_probe.hpp').read_text()
    assert 'bool metrics = false' in probe and 'captured[4*4] == 0' in probe
    assert 'diagnosed[component]-ordinary[component]' in probe and 'Mutually exclusive capture' in probe


if __name__ == '__main__':
    check_math()
    check_sources()
    print('PASS: P4.6 diagnostic encoding/source parity and analytical cost. Compilation/native: not_run.')
