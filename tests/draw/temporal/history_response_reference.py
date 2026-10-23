#!/usr/bin/env python3
"""P4.3 clipping/reactivity/effective-count reference; no native execution."""
from pathlib import Path
import math
import re
import struct
import sys

sys.dont_write_bytecode = True
from neighborhood_reference import gather, Receiver, EPSILON, LUMA

ROOT = Path(__file__).resolve().parents[3]


def statistic(rgb):
    return max(math.fsum(a*b for a, b in zip(rgb, LUMA)), 0)


def clip(history, stats, sigma, kind):
    if not stats or not stats['variance_valid']:
        return history, 0
    sigma_std = sigma*math.sqrt(stats['variance']+EPSILON)
    lo = max(stats['statistic_min'], stats['mean']-sigma_std)
    hi = min(stats['statistic_max'], stats['mean']+sigma_std)
    if not all(math.isfinite(v) for v in (lo, hi, sigma_std)) or lo > hi:
        return history, 0
    previous = history[0] if kind == 'scalar' else statistic(history[:3])
    target = min(hi, max(lo, previous))
    if kind == 'scalar':
        output = (target,)
    elif previous > EPSILON:
        output = tuple(min(65504, max(-65504, channel*(target/previous))) for channel in history[:3]) + (history[3],)
    else:
        output = tuple(min(high, max(low, channel)) for channel, low, high in zip(history[:3], stats['minimum'], stats['maximum'])) + (history[3],)
    delta = abs(previous-stats['mean'])
    reactive = min(1, max(0, (delta-sigma_std)/(delta+sigma_std+EPSILON)))
    return output, reactive


def resolve(current, history, count, maximum, stats, kind='scalar', enabled=True,
            support=1, minimum=.03125, reactive=.25, sigma=3, freeze=False, accepted=True):
    if not accepted:
        return current, 1, 1, 0
    n = min(count, maximum)
    if freeze and n >= maximum:
        return history, n, 0, 0
    alpha0 = 1/min(n+1, maximum)
    response, reactivity = clip(history, stats, sigma, kind) if enabled else (history, 0)
    alpha = min(1, max(alpha0, minimum, (1-support)*reactive, reactivity*reactive)) if enabled else alpha0
    if enabled and alpha > alpha0:
        n = min(n, max(1, 1/max(alpha, EPSILON)-1))
    if kind == 'reflection':
        confidence = current[3]*alpha + response[3]*(1-alpha)
        rgb = tuple((current[i]*current[3]*alpha + response[i]*response[3]*(1-alpha))/confidence
                    if confidence > 1e-8 else 0 for i in range(3))
        output = (*rgb, confidence)
    else:
        output = tuple(a*alpha+b*(1-alpha) for a, b in zip(current, response))
    return output, min(n+1, maximum), alpha, reactivity


def constant(value, kind='scalar', size=(3, 3)):
    return gather((0, 0), size, size, lambda _: value, lambda _: Receiver(), kind=kind)


def check_math():
    flat = constant((.75,))
    response, r = clip((.25,), flat, 3, 'scalar')
    assert response == (.75,) and .98 < r < 1
    output, count, alpha, _ = resolve((.75,), (.25,), 16, 32, flat, reactive=1)
    assert output == (.75,) and count == 2 and .98 < alpha < 1
    # Change is measured against the unclipped history, not the clipped .75.
    assert r > .98 and clip((.75,), flat, 3, 'scalar') == ((.75,), 0)
    uniform = constant((.5,))
    assert resolve((.5,), (.5,), 8, 32, uniform) == ((.5,), 9, 1/9, 0)
    single = constant((.75,), size=(1, 1))
    assert clip((.25,), single, 3, 'scalar') == ((.25,), 0)
    assert resolve((.75,), (.25,), 8, 32, single, minimum=0, reactive=0)[1] == 9
    assert clip((.25,), None, 3, 'scalar') == ((.25,), 0)
    # Noise/variance suppresses reaction, while sigma zero produces exact mean bounds.
    noisy = gather((0, 0), (2, 1), (2, 1), lambda p: (float(p[0]),), lambda _: Receiver())
    assert clip((1,), noisy, 3, 'scalar') == ((1,), 0)
    assert clip((1,), noisy, 0, 'scalar')[0] == (.5,)
    empty = dict(flat, mean=2, statistic_min=0, statistic_max=1)
    assert clip((.25,), empty, 0, 'scalar') == ((.25,), 0)
    # Disabled/frozen/rejected paths must not even inspect the supplied statistics.
    class Forbidden(dict):
        def __bool__(self):
            raise AssertionError('statistics must not be consumed')
    forbidden = Forbidden()
    assert resolve((1,), (.25,), 32, 32, forbidden, freeze=True) == ((.25,), 32, 0, 0)
    assert resolve((1,), (.25,), 1, 1, forbidden, freeze=True) == ((.25,), 1, 0, 0)
    assert resolve((1,), (.25,), 32, 32, forbidden, accepted=False) == ((1,), 1, 1, 0)
    assert resolve((1,), (0,), 3, 32, forbidden, enabled=False) == ((.25,), 4, .25, 0)
    assert resolve((1,), (0,), 1, 1, None)[0] == (1,)
    # Low geometric support/minimum floor can cap trust even without variance support.
    low = resolve((1,), (0,), 31, 32, None, support=.25)
    assert low[2] == .1875 and math.isclose(low[1], 16/3)
    assert resolve((1,), (0,), 31, 32, None, minimum=1, reactive=1)[1] == 2
    half = lambda x: struct.unpack('e', struct.pack('e', x))[0]
    assert 1 <= half(low[1]) <= 32
    black = constant((0, 0, 0, 1), 'gi')
    assert resolve((0, 0, 0, 1), (10, 5, 2, 1), 16, 32, black, kind='gi')[0] == (0, 0, 0, 1)
    red = constant((4, 0, 0, 1), 'gi')
    assert clip((0, 0, 0, 1), red, 3, 'gi')[0] == (4, 0, 0, 1)
    # Luminance-only scaling cannot identify equal-luminance chromatic change.
    blue = (0, 0, .2126/.0722, 1)
    red_one = constant((1, 0, 0, 1), 'gi')
    chromatic, r = clip(blue, red_one, 3, 'gi')
    assert chromatic[2] > 2.9 and r == 0
    hdr = constant((65504, 65504, 65504, 1), 'gi')
    bounded, _ = clip((65504, 0, 0, .25), hdr, 3, 'reflection')
    assert bounded[3] == .25 and all(math.isfinite(half(c)) for c in bounded)
    # Reflection misses do not darken retained hit RGB through alpha coupling.
    miss = resolve((999, 999, 999, 0), (8, 4, 2, .5), 3, 32, None, kind='reflection', minimum=0, reactive=0)
    assert miss[0] == (8, 4, 2, .375)
    hit = resolve((2, 4, 6, .5), (0, 0, 0, 0), 3, 32, None, kind='reflection', minimum=0, reactive=0)
    assert hit[0] == (2, 4, 6, .125)
    confidence_stats = constant((2, 4, 6, .5), 'reflection')
    changed, _ = clip((8, 4, 2, .25), confidence_stats, 3, 'reflection')
    assert changed[3] == .25


def check_sources():
    canonical = []
    for language in ('glsl', 'metal'):
        source = (ROOT / f'shaders/src/draw/lib/temporal_history_response.{language}').read_text()
        assert source.startswith('/*') and 'GNU General Public License' in source
        assert source.count('{') == source.count('}') and source.count('(') == source.count(')')
        assert 'if (!statistics.varianceValid)' in source
        assert source.index('float historicalStatistic') < source.index('float delta =')
        assert 'history.rgb*(clippedStatistic/historicalStatistic)' in source
        assert 'statistics.variance+CURRENT_STATISTICS_EPSILON' in source
        assert '(delta-sigmaStd)/(delta+sigmaStd+CURRENT_STATISTICS_EPSILON)' in source
        assert 'lower > upper' in source and '-65504.0' in source
        assert not re.search(r'(?:response\.history|history)\.a\s*=', source)
        body = source[source.index('// Local response only'):]
        canonical.append(body.replace('vec3', 'float3').replace('vec4', 'float4'))
        shared = (ROOT / f'shaders/src/draw/lib/temporal_reprojection.{language}').read_text()
        assert f'#include "temporal_history_response.{language}"' in shared
        assert 'if (alpha > alpha0)' in shared and 'oldCount = min(oldCount,trustCap)' in shared
        assert 'max(1.0,1.0/max(alpha,CURRENT_STATISTICS_EPSILON)-1.0)' in shared
        assert shared.index('if (dataBlock.adaptive.w > 0.5 && !frozen)') < shared.index('HistoryResponse response =')
        assert shared.index('previous = response.history') < shared.index('oldWeight = 1.0-alpha')
    assert canonical[0] == canonical[1]


if __name__ == '__main__':
    check_math()
    check_sources()
    print('PASS: P4.3 clipping/reactivity/trust-count references and source parity. Compilation/native: not_run.')
