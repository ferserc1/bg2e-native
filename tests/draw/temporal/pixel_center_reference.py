#!/usr/bin/env python3
"""Numerical regression for history transport; does not execute GPU shaders."""
from fractions import Fraction
import math
from pathlib import Path
import re
import unittest


def receiver_center(pixel, signal_extent, surface_extent):
    uv = Fraction(2 * pixel + 1, 2 * signal_extent)
    source_pixel = int(uv * surface_extent)
    return uv, Fraction(2 * source_pixel + 1, 2 * surface_extent)


def history_texel(pixel, signal_extent, surface_extent, displacement=Fraction(0)):
    signal_uv, receiver_uv = receiver_center(pixel, signal_extent, surface_extent)
    previous_receiver_uv = receiver_uv + displacement
    history_uv = signal_uv + (previous_receiver_uv - receiver_uv)
    return history_uv * signal_extent - Fraction(1, 2)


class PixelCenterRegression(unittest.TestCase):
    def test_preprocessor_directives_start_on_their_own_line(self):
        root = Path(__file__).resolve().parents[3]
        for name in ('temporal', 'temporal_reprojection'):
            for language in ('glsl', 'metal'):
                path = root / f'shaders/src/draw/lib/{name}.{language}'
                for number, line in enumerate(path.read_text().splitlines(), 1):
                    # An inline #else silently removes the entire camera branch
                    # when TEMPORAL_WORLD=0, hiding its history declarations.
                    if re.search(r'#\s*(?:if|ifdef|ifndef|elif|else|endif)\b', line):
                        self.assertTrue(line.lstrip().startswith('#'), f'{path}:{number}')

    def test_static_identity_at_all_scales(self):
        # Include odd dimensions, nonintegral ratios, full-resolution signals,
        # and reduced reflection signals at every reported upscale preset.
        for output in (853, 1600, 1921):
            for percent in (35, 50, 55, 60, 100):
                surface = output * percent // 100
                for signal in (surface, max(1, surface // 2), max(1, surface // 4)):
                    for pixel in range(signal):
                        self.assertEqual(history_texel(pixel, signal, surface), pixel)

    def test_jitter_and_motion_are_not_cancelled(self):
        for signal, surface in ((200, 400), (301, 853), (427, 853)):
            for pixel in (0, signal // 2, signal - 1):
                # Opposite jitter phases and real motion must survive transport.
                for displacement in (Fraction(-1, 3 * surface), Fraction(1, 3 * surface),
                                     Fraction(7, surface), Fraction(-7, surface)):
                    expected = pixel + displacement * signal
                    self.assertEqual(history_texel(pixel, signal, surface, displacement), expected)

    def test_frozen_history_does_not_advect_a_reflection(self):
        # A linear radiance ramp is reproduced exactly by bilinear sampling.
        # Feed it back as frozen history: any steady sampling bias becomes drift.
        signal, surface, pixel = 200, 400, 80
        signal_uv, receiver_uv = receiver_center(pixel, signal, surface)
        old_offset = float((receiver_uv - signal_uv) * signal)
        self.assertEqual(old_offset, 0.25)
        old_value = corrected_value = float(pixel)
        for _ in range(128):
            old_value += old_offset
            corrected_value += float(history_texel(pixel, signal, surface) - pixel)
        self.assertEqual(old_value - pixel, 32)
        self.assertEqual(corrected_value, pixel)

        # Rounding fractional jitter to nearest texels introduces another bias.
        shifts = (0.4, 0.4, -0.8)
        self.assertTrue(math.isclose(sum(shifts), 0, abs_tol=1e-12))
        self.assertEqual(sum(math.floor(shift + 0.5) for shift in shifts), -1)


if __name__ == "__main__":
    unittest.main()
