#!/usr/bin/env python3
"""P4.5 temporal-stream reference, not execution of C++/shaders or GPU ownership."""
from dataclasses import dataclass
from pathlib import Path
import math
import re
import statistics
import sys

sys.dont_write_bytecode = True
from neighborhood_reference import gather, Receiver
from history_response_reference import clip, statistic
from moments_reference import bounded, half, reactivity as moment_reactivity, spatial, valid

ROOT = Path(__file__).resolve().parents[3]


@dataclass(frozen=True)
class State:
    frame: int
    recording: int
    submitted: bool
    mode: int
    color: tuple
    count: float
    moments: tuple | None
    rolling: int


class Stream:
    def __init__(self):
        self.histories = {}
        self.generation = 1

    def invalidate(self):
        self.histories.clear()
        self.generation += 1

    def step(self, frame, neighbors, kind='scalar', key=(0, 0), mode=5, available=True,
             enabled=True, maximum=32, freeze=False, motion=False, support=1,
             accepted=True, isolated=False, submitted=True, recording=None):
        recording = frame if recording is None else recording
        mode = 3 if not enabled else (4 if mode == 5 and not available else mode)
        raw = tuple(half(v) for v in neighbors[4])
        covered = all(math.isfinite(v) for v in raw) and (kind != 'gi' or raw[3] > .5)
        current = (min(1, max(0, raw[0])),) if kind == 'scalar' else (*raw[:3], 1 if kind == 'gi' else min(1, max(0, raw[3])))
        previous = self.histories.get((kind, *key))
        accepted = bool(accepted and covered and support > 1e-6 and previous and previous.count >= 1 and
                        previous.frame+1 == frame and previous.mode == mode and
                        (previous.submitted or previous.recording == recording))
        rolling_previous = accepted and previous.rolling > 0
        rolling = 3*maximum if motion else (previous.rolling-1 if rolling_previous else 0)
        alpha, reaction = 1.0, 0.0
        pair = None
        current_stat = current[0] if kind == 'scalar' else (0 if kind == 'reflection' and current[3] <= 1e-6 else statistic(current[:3]))
        if mode == 5 and covered:
            value = bounded(current_stat, kind == 'scalar')
            pair = (value, value*value)
        if accepted:
            n = min(previous.count, maximum)
            alpha0 = 1/min(n+1, maximum)
            frozen = freeze and not motion and not rolling_previous and n >= maximum
            old = previous.color
            if frozen:
                alpha, n, output = 0, n, old
                pair = previous.moments
            else:
                alpha = alpha0
                if mode >= 4:
                    values = [tuple(half(v) for v in sample) for sample in neighbors]
                    guide = lambda p: Receiver(token=7 if not isolated or p == (1, 1) else 8)
                    stats = gather((1, 1), (3, 3), (3, 3), lambda p: values[p[1]*3+p[0]], guide, kind=kind)
                    old, reaction = clip(old, stats, 3, kind)
                    if mode == 5:
                        luminances = []
                        for p, _ in stats['visits'] if stats else ():
                            value = values[p[1]*3+p[0]]
                            luminances.append(min(1, max(0, value[0])) if kind == 'scalar' else statistic(value[:3]))
                        mean, variance = spatial(luminances, kind == 'scalar') if luminances else (0, 0)
                        reaction = moment_reactivity(previous.moments, mean, variance, n, enough=bool(stats and stats['variance_valid']))
                    alpha = min(1, max(alpha0, .03125, (1-support)*.25, reaction*.25))
                    if alpha > alpha0:
                        n = min(n, max(1, 1/max(alpha, 1e-6)-1))
                if kind == 'reflection':
                    confidence = current[3]*alpha+old[3]*(1-alpha)
                    rgb = tuple((current[i]*current[3]*alpha+old[i]*old[3]*(1-alpha))/confidence if confidence > 1e-8 else 0 for i in range(3))
                    output = (*rgb, confidence)
                else:
                    output = tuple(a*alpha+b*(1-alpha) for a, b in zip(current, old))
                if mode == 5:
                    pair = tuple(a*alpha+b*(1-alpha) for a, b in zip(pair, previous.moments))
            count = min(n+1, maximum)
        else:
            output, count = current, 1
        if not covered:
            output = (1,) if kind == 'scalar' else (0, 0, 0, 0)
            count = 0
            if mode == 5:
                pair = (0, 0)
        output = tuple(half(v) for v in output)
        pair = tuple(half(v) for v in pair) if pair is not None else None
        result = State(frame, recording, submitted, mode, output, half(count), pair, rolling)
        self.histories[(kind, *key)] = result
        return result, alpha, reaction


def uniform(value):
    return [value]*9


def check_math():
    step_frames = {}
    for mode in (3, 4, 5):
        stream = Stream()
        retained = []
        for frame in range(32):
            retained.append(stream.step(frame, uniform((.25,)), mode=mode)[0])
        errors = []
        for frame in range(32, 192):
            state, _, _ = stream.step(frame, uniform((.75,)), mode=mode)
            errors.append(abs(state.color[0]-.75))
        step_frames[mode] = next(i+1 for i in range(len(errors)-2) if max(errors[i:i+3]) <= .01)
        assert stream.generation == 1 and retained[0].color == (.25,)
        assert all(math.isfinite(e) for e in errors)
    assert step_frames[4] < step_frames[3] and step_frames[5] < step_frames[3]
    noise_errors = {}
    for mode in (3, 4, 5):
        stream = Stream()
        errors = []
        for frame in range(96):
            noisy = [(.25 if (frame+p)%2 == 0 else .75,) for p in range(9)]
            state, _, _ = stream.step(frame, noisy, mode=mode)
            assert .25 <= state.color[0] <= .75 and 1 <= state.count <= 32
            if frame >= 32:
                errors.append(state.color[0]-.5)
        noise_errors[mode] = math.sqrt(statistics.mean(e*e for e in errors))
        assert noise_errors[mode] < .02 and stream.generation == 1
    for mode in (4, 5):
        stream = Stream()
        stream.step(0, uniform((0,)), mode=mode, isolated=True)
        one, alpha, r = stream.step(1, uniform((1,)), mode=mode, isolated=True)
        assert one.color == (.5,) and alpha == .5 and r == 0
        for frame in (2, 3):
            state, _, _ = stream.step(frame, uniform((0, 0, 0, 1)), kind='gi', mode=mode)
            assert state.color == (0, 0, 0, 1)
        bright, _, _ = stream.step(4, uniform((65504, 65504, 65504, 1)), kind='gi', mode=mode)
        assert all(math.isfinite(v) and abs(v) <= 65504 for v in bright.color)
        if mode == 5:
            assert valid(bright.moments)
        outlier = uniform((0, 0, 0, 1))[:]
        outlier[4] = (65504, 65504, 65504, 1)
        peak, _, _ = stream.step(5, outlier, kind='gi', mode=mode)
        assert all(math.isfinite(v) for v in peak.color)
        stream.step(6, uniform((8, 4, 2, .5)), kind='reflection', mode=mode)
        miss, _, _ = stream.step(7, uniform((65504, 65504, 65504, 0)), kind='reflection', mode=mode)
        assert miss.color == (8, 4, 2, .25)
        hit, _, _ = stream.step(8, uniform((2, 4, 6, .5)), kind='reflection', mode=mode)
        assert abs(hit.color[3]-1/3) < .001
        # Literal freeze skips response; motion renews exactly 3*maxFrames later records.
        stream = Stream()
        stream.step(0, uniform((.25,)), maximum=2, freeze=True, mode=mode)
        stream.step(1, uniform((.25,)), maximum=2, freeze=True, mode=mode)
        frozen, alpha, _ = stream.step(2, uniform((.75,)), maximum=2, freeze=True, mode=mode)
        assert frozen.color == (.25,) and alpha == 0
        thaw, alpha, _ = stream.step(3, uniform((.75,)), maximum=2, freeze=True, mode=mode, motion=True)
        assert thaw.color == (.75,) and alpha > 0 and thaw.rolling == 6
        for frame in range(4, 10):
            state, alpha, _ = stream.step(frame, uniform((.75,)), maximum=2, freeze=True, mode=mode)
            assert alpha > 0 and state.rolling == 9-frame
        assert stream.step(10, uniform((.25,)), maximum=2, freeze=True, mode=mode)[1] == 0
        dropped, _, _ = stream.step(11, uniform((.25,)), mode=mode, submitted=False)
        restarted, _, _ = stream.step(12, uniform((.75,)), mode=mode)
        assert restarted.color == (.75,) and restarted.count == 1
        assert dropped.color == (.25,)  # Immutable reference snapshot, not a GPU ownership proof.
        gap, _, _ = stream.step(14, uniform((.5,)), mode=mode)
        assert gap.count == 1 and gap.color == (.5,)
        one, _, _ = stream.step(15, uniform((.75,)), mode=mode, maximum=1)
        assert one.count == 1 and one.color == (.75,)
        invalid, _, _ = stream.step(16, uniform((math.nan,)), mode=mode)
        assert invalid.count == 0 and invalid.color == (1,)
        assert stream.step(17, uniform((.25,)), mode=mode)[0].count == 1
    fallback = Stream().step(0, uniform((.25,)), mode=5, available=False)[0]
    assert fallback.mode == 4 and fallback.moments is None
    disabled = Stream().step(0, uniform((.25,)), mode=5, enabled=False)[0]
    assert disabled.mode == 3 and disabled.moments is None
    for slots in (2, 3):
        stream = Stream()
        outputs = []
        for frame in range(2*slots):
            for key, value in (((0, 0), .25), ((1, 0), .5), ((0, 1), 1)):
                outputs.append(stream.step(frame, uniform((value,)), key=key)[0])
        assert len(stream.histories) == 3 and {s.color for s in outputs} == {(.25,), (.5,), (1,)}
        assert outputs[0].moments == (.25, .0625)
    print('Reference-only scalar step first frame of three-sample <=.01 window:', step_frames)
    print('Reference-only deterministic alternating-noise tail RMSE:', noise_errors)


def check_sources():
    for name in ('adaptive_probe.hpp', 'queued_reprojection_probe.hpp'):
        source = (ROOT / 'tests/draw/temporal' / name).read_text()
        assert source.startswith('/*') and 'GNU General Public License' in source
        tokens = re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
        assert tokens.count('{') == tokens.count('}') and tokens.count('(') == tokens.count(')'), name
        for include in re.findall(r'#include "([^\"]+)"', source):
            assert (ROOT / 'tests/draw/temporal' / include).exists()
    adaptive = (ROOT / 'tests/draw/temporal/adaptive_probe.hpp').read_text()
    for marker in ('forceMomentFailure', 'neighborhood-only v4 fallback', 'isolateCenter', 'frame < 24',
                   '0x7bff', 'chromatic', 'TemporalSignal::ReflectedRadiance', 'setMaxFrames(1)'):
        assert marker in adaptive
    queued = (ROOT / 'tests/draw/temporal/queued_reprojection_probe.hpp').read_text()
    assert 'bool adaptive = false, bool moments = false' in queued
    assert 'for (const uint32_t slots : {2u,3u})' in queued
    assert queued.index('device.graphicsQueue().submit(command.get())') < queued.index('auto read =')
    assert 'command.reset();' in queued and 'extraMoments' in queued
    assert 'result.extraMoments[0] != result.extraMoments[1]' in queued


if __name__ == '__main__':
    check_math()
    check_sources()
    print('PASS: P4.5 sequence references/source fixtures. C++/shader/native/visual/performance: not_run.')
