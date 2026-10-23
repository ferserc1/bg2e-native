#!/usr/bin/env python3
"""P5.1 integration source guards; does not compile or execute native code."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def read(path):
    return (ROOT / path).read_text()


def function(source, name):
    start = source.index(name)
    start = source.index('{', start)
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    model = read('apps/model_edit/src/AppDelegate.cpp')
    actions = read('apps/model_edit/src/EditorActions.cpp')
    mouse = function(model, 'void AppDelegate::queueInput(')
    transform = function(actions, 'void AppDelegate::queueTransformEdit(')
    for source in (mouse, transform):
        for forbidden in ('invalidateHistories(', 'attachScene(', 'setScene(', 'notifyEdited('):
            assert forbidden not in source, forbidden
        assert 'document()->generation != generation' in source
        assert 'weak.lock()' in source and 'enqueueRendererUpdate(' in source
    assert 'std::isfinite(matrix[column][row])' in transform
    assert 'node->transform()->setMatrix(matrix)' in transform
    assert 'invalidateLightmapSources()' in transform and '_stage->markModified()' in transform
    assert '_stage->contains(node.get())' in transform and '!node->transform()' in transform
    assert 'queueTransformEdit(target->shared_from_this(), matrix)' in read('apps/model_edit/src/EditorTools.cpp')
    general = function(actions, 'void AppDelegate::notifyEdited(')
    assert 'renderer.invalidateHistories()' in general and 'renderer.attachScene(scene())' in general
    assert 'invalidateHistories(' not in function(read('apps/bg2e_composer/src/AppDelegate.cpp'), 'void AppDelegate::queueInput(')

    renderer = read('lib/src/bg2e/draw/RendererDeferred.cpp')
    assert 'slot.transparentTemporalSurface = _temporalSurface->record' in renderer
    assert 'temporalSurfaceMetadata(frame.frameSlot,layer)' in renderer
    assert 'slot.opaqueTemporalSurface.motion,slot.opaqueTemporalSurface.token' in renderer
    assert 'slot.transparentTemporalSurface = {}' in renderer
    accumulator = read('lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp')
    assert 'previous->recording != input.command.recordingCompletion()' in accumulator
    assert 'previous->sequence == input.sceneFrameNumber' in accumulator
    assert 'previous->metadataMode != metadataMode' in accumulator
    assert 'input.surface.profile == SurfaceProfile::CameraDepth && input.view.reproject' in accumulator
    assert 'const bool adaptive = rigid && _settings.adaptiveEnabled && adaptiveAvailable()' in accumulator
    correspondence = read('lib/src/bg2e/draw/TemporalCorrespondence.cpp')
    assert '_recorded->recording->wasSubmitted()' in correspondence
    loop = read('lib/src/bg2e/draw/RenderLoop.cpp')
    assert loop.index('queue.submit(&command)') < loop.index('++_renderedSceneCount')
    print('P5.1 integration source guards passed (native execution not_run)')


if __name__ == '__main__':
    main()
