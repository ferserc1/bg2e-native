#!/usr/bin/env python3
"""Buffer-inspection math/source checks. Does not compile or execute native code."""
import math
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[3]


def read(path):
    return (ROOT / path).read_text()


class InspectionContracts(unittest.TestCase):
    def test_ui_view_order_matches_renderer(self):
        header = read('lib/include/bg2e/draw/RendererDebug.hpp')
        members = header.split('enum class RendererDebugView : uint32_t')[1].split('};')[0]
        views = re.findall(r'\b[A-Z]\w*\b', members)
        ui = read('apps/bg2e_composer/src/EditorDebug.cpp')
        labels = re.findall(r'"([^"\n]*)"', ui.split('views{')[1].split('};')[0])
        self.assertEqual(len(views), len(labels))
        self.assertEqual(labels[views.index('GlobalIllumination')], 'Global illumination')
        self.assertEqual(labels[views.index('AfterUpscale')], 'Color after upscale')
        self.assertEqual(labels[views.index('TemporalCount')], 'Temporal effective count')
        self.assertIn('bool enabled = false;', header)
        self.assertNotIn('Preferences', ui)
        self.assertIn('beginDisabled(!options.enabled)', ui)
        self.assertIn('enqueueRendererUpdate', ui)

    def test_observation_does_not_replace_upscale_input(self):
        source = read('lib/src/bg2e/draw/RendererDeferred.cpp')
        self.assertIn('UpscalerInput upscale{frame, *current,', source)
        self.assertIn('(!slot.diagnostic || _debugOptions.enabled)', source)
        self.assertIn('else if (!_debugOptions.enabled)', source)
        self.assertLess(source.index('RendererDebugStage::Original,signal,layer'),
                        source.index('_configuration.temporal->record({'))
        self.assertLess(source.index('RendererDebugStage::Accumulated,*accumulated.image'),
                        source.index('_configuration.denoise->record({'))
        self.assertLess(source.index('recordGIUpsampling(frame.commandBuffer,selectedOwner'),
                        source.index('RendererDebugStage::Filtered,*signal,layer',
                                     source.index('recordGIUpsampling(frame.commandBuffer,selectedOwner')))
        self.assertLess(source.index('_configuration.upscaler->record(upscale)'),
                        source.index('RendererDebugView::AfterUpscale'))
        self.assertLess(source.index('recordDebugFinal(frame)'), source.index('slot.finalRecorded = true'))

    def test_disabled_resources_and_transparent_exclusion(self):
        source = read('lib/src/bg2e/draw/RendererDebug.cpp')
        begin = source.split('void RendererDeferred::beginDebugFrame')[1].split('void RendererDeferred::captureDebug')[0]
        self.assertLess(begin.index('!_debugOptions.enabled'), begin.index('make_shared<DebugState>'))
        self.assertIn('_debug.reset()', source)
        self.assertIn('releaseDiagnosticCaptures()', source)
        temporal = read('lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp')
        release = temporal.split('void TemporalAccumulator::releaseDiagnosticCaptures()')[1].split('void TemporalAccumulator::invalidate()')[0]
        for expression in ('waitIdleForResourceUpdate()', 'slot.invocations.clear()',
                           'history->acceptance.reset()', 'history->metrics.reset()', '_pool.reset()'):
            self.assertIn(expression, release)
        self.assertNotIn('_histories.clear()', release)
        queue = read('lib/src/bg2e/draw/RenderQueue.cpp').split('void RenderQueue::excludeTransparent()')[1].split('void RenderQueue::sort')[0]
        for expression in ('_opaque[i].objectId = uint32_t(i + 1)', '_items = _opaque',
                           '_transparent.clear()', '_solidTransparent.clear()'):
            self.assertIn(expression, queue)
        renderer = read('lib/src/bg2e/draw/RendererDeferred.cpp')
        self.assertLess(renderer.index('slot.queue.excludeTransparent()'), renderer.index('prepareTemporalSignature(slot.queue)'))

    def test_hdr_is_preserved_until_display(self):
        # Clipping an HDR snapshot before applying exposure would hide differences.
        for radiance in (.125, 1, 8):
            displayed = min(1, max(0, radiance * 2**-4))
            self.assertAlmostEqual(displayed, radiance/16)
        for language in ('glsl', 'metal'):
            source = read(f'shaders/src/draw/buffer_inspection/color.frag.{language}')
            self.assertIn('if (pc.rangeMode.w >= 0.0)', source)
            self.assertIn('pc.rangeMode.w < 0.0 ? value', source)
            self.assertNotIn('.sample(', source)
        self.assertIn('depth2d<float>', read('shaders/src/draw/buffer_inspection/depth.frag.metal'))

    def test_linear_depth_roundtrip(self):
        near, far = .1, 1000
        a, b = far/(near-far), near*far/(near-far)
        for distance in (near, .2, 1, 10, 100, far):
            device_depth = -a + b/distance
            reconstructed = abs(b/(device_depth+a))
            self.assertTrue(math.isclose(reconstructed, distance, rel_tol=1e-10))
            grey = min(1, max(0, (reconstructed-.1)/(10-.1)))
            self.assertTrue(0 <= grey <= 1)


if __name__ == '__main__':
    unittest.main()
