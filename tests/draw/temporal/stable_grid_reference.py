#!/usr/bin/env python3
"""Stable GI feedback regressions and shader source checks; no native execution."""
import math
from pathlib import Path
import re
import random
import unittest

ROOT = Path(__file__).resolve().parents[3]


def halton(index, base):
    result, factor = 0.0, 1.0/base
    while index:
        result += (index % base)*factor
        index //= base
        factor /= base
    return result


def linear(values, coordinate):
    base = math.floor(coordinate)
    fraction = coordinate-base
    return sum(weight*values[max(0, min(len(values)-1, p))]
               for p, weight in ((base, 1-fraction), (base+1, fraction)))


def sequence(stable, freeze, scale=1, length=1024):
    history, previous_jitter = None, 0
    contrasts = {}
    for frame in range(length):
        jitter = (halton(frame % 32+1, 3)-.5)*scale
        current = [1-.5*math.exp(-.5*((i-jitter-32)/1.0)**2) for i in range(65)]
        if stable:
            # Filter only the newly generated, jittered signal onto the fixed grid.
            current = [linear(current, i+jitter) for i in range(len(current))]
        if history is None:
            result = current
        else:
            alpha = 0 if freeze and frame >= 32 else 1/min(frame+1, 32)
            result = [alpha*value+(1-alpha)*(history[i] if stable else
                      linear(history, i+previous_jitter-jitter)) for i, value in enumerate(current)]
        history, previous_jitter = result, jitter
        if frame in (31, 255, length-1):
            contrasts[frame+1] = 1-min(history)
    return contrasts


def preprocess(path, definitions=None):
    """Read just active private shader branches, expanding local includes."""
    definitions = {} if definitions is None else definitions
    output, stack, active = [], [], True
    for line in path.read_text().splitlines():
        directive = line.strip()
        if directive.startswith('#ifndef '):
            condition = directive.split()[1] not in definitions
            stack.append((active, condition)); active = active and condition
        elif directive.startswith('#if '):
            expression = directive[4:].strip()
            expression = re.sub(r'\b[A-Za-z_]\w*\b', lambda m: str(definitions.get(m[0], 0)), expression)
            expression = expression.replace('!', ' not ')
            condition = bool(eval(expression, {'__builtins__': {}}, {}))
            stack.append((active, condition)); active = active and condition
        elif directive == '#else':
            parent, condition = stack[-1]; active = parent and not condition
        elif directive == '#endif':
            parent, _ = stack.pop(); active = parent
        elif active and directive.startswith('#define '):
            fields = directive.split()
            if len(fields) > 2 and fields[2].isdigit():
                definitions[fields[1]] = int(fields[2])
        elif active and directive.startswith('#include "'):
            name = directive.split('"')[1]
            output.append(preprocess((path.parent/name).resolve(), definitions))
        elif active and not directive.startswith('#'):
            output.append(line)
    assert not stack, str(path)
    return '\n'.join(output)


class StableGIRegression(unittest.TestCase):
    def test_no_progressive_loss_with_or_without_freeze(self):
        for freeze in (False, True):
            for scale in (.35, .5, .55, 1, 1.5):
                result = sequence(True, freeze, scale)
                self.assertGreater(result[1024], .35)
                if freeze:
                    self.assertAlmostEqual(result[32], result[1024], places=8)
                else:
                    self.assertLess(abs(result[32]-result[1024]), .01)
                    self.assertLess(abs(result[256]-result[1024]), .0001)
        old_frozen = sequence(False, True)
        self.assertLess(old_frozen[1024], old_frozen[32]/4)
        old_rolling = sequence(False, False)
        self.assertLess(old_rolling[1024], sequence(True, False)[1024]-.1)

    def test_noise_is_still_accumulated(self):
        rng = random.Random(7)
        history = None
        raw_errors = []
        for frame in range(256):
            jitter = halton(frame % 32+1, 3)-.5
            raw = [1+rng.uniform(-.2,.2) for _ in range(65)]
            current = [linear(raw, i+jitter) for i in range(65)]
            alpha = 1/min(frame+1,32)
            history = current if history is None else [alpha*x+(1-alpha)*y for x,y in zip(current,history)]
            raw_errors.extend((x-1)**2 for x in current)
        raw_rmse = math.sqrt(sum(raw_errors)/len(raw_errors))
        history_rmse = math.sqrt(sum((x-1)**2 for x in history)/len(history))
        self.assertLess(history_rmse, raw_rmse/3)

    def test_discontinuous_receivers_do_not_mix(self):
        def gather(taps, normal, token):
            accepted = [(value, weight) for value, weight, n, t in taps if n == normal and t == token]
            return sum(v*w for v, w in accepted)/sum(w for _, w in accepted)
        samples = [(0.05, .25, 'floor', 1), (20, .75, 'cube', 2)]
        self.assertAlmostEqual(gather(samples, 'floor', 1), .05)
        self.assertAlmostEqual(gather(samples, 'cube', 2), 20)

    def test_motion_and_grid_changes(self):
        # Jitter cancels; camera/object motion must not cancel with it.
        for signal in (101, 200, 427):
            uv = 30.5/signal
            for jitter in (-.4/640, .3/640):
                receiver_uv = uv+jitter
                for motion in (-.2/640, 0, .8/640):
                    previous_receiver_uv = receiver_uv-jitter+motion
                    history_uv = uv+previous_receiver_uv-(receiver_uv-jitter)
                    self.assertAlmostEqual((history_uv-uv)*signal, motion*signal)
        cpp = (ROOT/'lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp').read_text()
        self.assertIn('previous->stableGrid != stableGrid', cpp)
        self.assertIn('previous->surfaceExtent != input.surface.extent()', cpp)
        self.assertIn('previous->view.projection != input.view.projection', cpp)
        self.assertIn('previous->recording->wasSubmitted()', cpp)

    def test_all_private_variants_expand_and_have_balanced_syntax(self):
        files = list((ROOT/'shaders/src/draw/temporal_stable_gi').glob('*.comp.*'))
        self.assertEqual(len(files), 24)  # 11 accumulation variants plus presentation, both backends.
        for path in files:
            text = preprocess(path)
            self.assertNotRegex(text, r'\b(?:EQUAL|OOBMETA|WRITE|STATIC)\b|\bREAD\(')
            self.assertNotRegex(text, r'\S[^\n]*#\s*(?:else|endif)')
            clean = re.sub(r'/\*.*?\*/|//[^\n]*|"[^"\n]*"', '', text, flags=re.S)
            stack, pairs = [], {')': '(', ']': '[', '}': '{'}
            for character in clean:
                if character in '([{': stack.append(character)
                elif character in ')]}':
                    self.assertTrue(stack, str(path))
                    self.assertEqual(stack.pop(), pairs[character], str(path))
            self.assertFalse(stack, str(path))
            if path.stem != 'present.comp':
                self.assertIn('stableJitter;', text)
                self.assertIn('stableGICurrent(', text)
                self.assertIn('historyUV = uv;', text)
                self.assertRegex(text, r'(?:q|historyPixel) = (?:vec2|float2)\(pixel\)')
                self.assertEqual(len(re.findall(r'(?:vec4|float4) stableJitter;', text)), 1)

    def test_presentation_never_enters_feedback(self):
        cpp = (ROOT/'lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp').read_text()
        self.assertIn('(stableGrid ? previous->view.projection : previous->view.jitteredProjection)', cpp)
        self.assertIn('std::memcpy(uniforms.data()+uniformBytes,&stableJitter', cpp)
        self.assertIn('return {presented.get(),presented,next->count}', cpp)
        self.assertNotIn('next->signal = presentStableGI', cpp)
        self.assertNotIn('next->moments = presentStableGI', cpp)
        self.assertIn('stableGrid && input.view.jitteredProjection != input.view.projection', cpp)
        present = (ROOT/'lib/src/bg2e/draw/pipeline/TemporalStableGI.cpp').read_text()
        self.assertIn('invocation.stableOwners.push_back(signal)', present)
        self.assertIn('invocation.stableBindings.push_back(pooled)', present)


if __name__ == '__main__':
    unittest.main()
