#!/usr/bin/env python3
"""Phase 3 source/cost audit; no C++/shader execution or measured GPU results."""
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from footprint_reference import check as check_reference
from feature_source import without_features

ROOT = Path(__file__).resolve().parents[3]


def read(name):
    return (ROOT / name).read_text()


def function(source, name):
    start = source.index('{', source.index(name))
    end, depth = start + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def without_diagnostics(source):
    # Diagnostic blocks in this paired family have no nested preprocessor branches.
    return re.sub(r'#if TEMPORAL_DIAGNOSTIC\n.*?#endif\n', '', source, flags=re.S)


def check_source():
    for language in ('glsl', 'metal'):
        source = read(f'shaders/src/draw/lib/temporal_reprojection.{language}')
        core = without_diagnostics(without_features(source, {'TEMPORAL_MOMENTS', 'TEMPORAL_METRICS'}))
        assert 'rejectionReason' not in core and 'reasonColor' not in core
        assert 'for (int tap = 0; tap < 4; ++tap)' in core
        assert core.count('for (') == 1 and 'while (' not in core
        assert core.index('accepted = accepted && all(') < core.index('floor(q)')
        assert core.index('if (weight <= 0.0') < core.index('uint previousToken =')
        assert core.index('previousToken != token') < core.index('bool candidateValid =')
        assert core.index('if (!candidateValid)') < core.index('historySum +=')
        assert 'candidateUV*' in core and 'previousUV*' not in core[core.index('metadataPixel ='):core.index('uint previousToken =')]
        assert 'validWeight > 1e-6 && finite4(historySum)' in core
        assert 'minimumCount = min(minimumCount,previousNormal.w)' in core
        assert 'float minimumCount = float(dataBlock.counts.y)' in core
        assert core.count('count = min(oldCount+1.0,float(dataBlock.counts.y))') == 1
        assert core.index('if (accepted)', core.index('geometricConfidence = accepted')) < core.index('float oldWeight')
        assert 'previousNormal.w >= 1.0' in core
        assert 'expectedDepth > 0.0 && candidateDepth > 0.0' in core
        assert 'candidate.rgb *= candidate.a' in core
        assert 'dataBlock.counts.x != 1u || candidate.a > 0.5' in core
        assert 'previous.a > 1e-8 ? previous.rgb/previous.a' in core
        assert 'previous.rgb*previous.a*oldWeight' in core
        assert 'count = 0.0' in core and 'covered ? 1.0 : 0.0' in core
        if language == 'glsl':
            assert 'textureSize(usampler2D(previousTokenImage,nearestSampler),0)' in core
        else:
            assert 'previousTokenImage.get_width(),previousTokenImage.get_height()' in core
        # Diagnostic reason selection cannot change the accumulation decision.
        for block in re.findall(r'#if TEMPORAL_DIAGNOSTIC\n(.*?)#endif', source, flags=re.S):
            assert not re.search(r'(?<![\w.])(?:accepted|result|count|current|previous|validWeight)\s*=(?!=)', block)
    accumulator = read('lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp')
    record = function(accumulator, 'TemporalAccumulator::record(')
    display = function(accumulator, 'TemporalAccumulator::recordDiagnostic(')
    assert 'base::CpuProfileScope profile("effect.temporal.record")' in record
    assert 'if (diagnose || metrics)\n    {\n        diagnosticCpu.emplace(' in record
    assert 'diagnosticGpu.emplace(input.command,"effect.temporal.diagnosticCapture"' in record
    assert 'if (diagnose)\n    {\n        next->acceptance = ' in record
    assert display.index('TemporalDiagnosticView::Off') < display.index('effect.temporal.diagnosticDisplay')
    assert display.index('if (!surfaceData)') < display.index('effect.temporal.diagnosticDisplay')
    for source in (record, display):
        for forbidden in ('waitIdle(', 'immediateSubmit(', 'readPixels(', '->completed('):
            assert forbidden not in source
    assert 'invocation.previous = previous' in record and 'invocation.next = next' in record
    assert record.index('slot.invocations.push_back') < record.index('input.command.dispatch(')
    assert display.index('slot.invocations.push_back') < display.index('command.dispatch(')
    assert 'next->metadata.motion.reset()' in record
    assert 'previous->recording->wasSubmitted()' in record
    assert 'previous->metadataMode != metadataMode' in record
    assert 'previous->signal->size() != input.signal.size()' in record
    renderer = read('lib/src/bg2e/draw/RendererDeferred.cpp')
    assert '"effect.temporal.record"' in renderer and '"renderer.temporalDiagnostic"' in renderer
    pool = read('lib/src/bg2e/draw/pipeline/TemporaryResourcePool.cpp')
    assert 'image.use_count() == 1' in pool and 'entry.completion->completed()' in pool
    assert 'temporary.retainedImageBytes' in pool


def costs(width, height, depth_bytes=4):
    """One unique image set, tightly packed bytes, excludes driver/pool overhead."""
    if width <= 0 or height <= 0 or depth_bytes not in (2, 4):
        raise ValueError('Positive extent and D16/D32 depth required')
    pixels = width * height
    return {'scalar_history': 26 * pixels, 'hdr_history': 32 * pixels,
            'metadata_domain': (32 + depth_bytes) * pixels,
            'history_metadata_subset': (28 + depth_bytes) * pixels,
            'acceptance': 8 * pixels, 'display': 8 * pixels}


def check_costs():
    a, b = costs(1920, 1080), costs(3840, 2160)
    assert all(b[key] == 4 * value for key, value in a.items())
    assert a['scalar_history'] == 53_913_600 and a['hdr_history'] == 66_355_200
    assert a['metadata_domain'] == 74_649_600 and a['acceptance'] == 16_588_800
    assert costs(1920, 1080, 2)['metadata_domain'] == 70_502_400
    # One domain shared by three signals: count metadata once, not once per signal.
    one_generation = a['scalar_history'] + 2*a['hdr_history'] + a['metadata_domain']
    assert one_generation == 261_273_600
    assert one_generation + a['acceptance'] + a['display'] == 294_451_200
    print('Analytical bytes at 1080p (one image set, not measured resident memory):', a)


if __name__ == '__main__':
    check_reference()
    check_source()
    check_costs()
    print('PASS: Phase 3 source/math/cost audit. Compilation/native/visual/timing: not_run.')
