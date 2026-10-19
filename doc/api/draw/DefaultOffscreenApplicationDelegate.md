# DefaultOffscreenApplicationDelegate

Header: `<bg2e/draw/DefaultOffscreenApplicationDelegate.hpp>`.
Namespace: `bg2e::draw`.

This public non-template class derives from `DefaultRenderLoopDelegate` and
`app::DrawOffscreenApplicationDelegate`. Its constructor accepts
`std::unique_ptr<Renderer>` and an optional `RendererConfiguration`. The existing
scene delegate owns that renderer and retains the scene. Override `createScene()`
or queue `setScene` before the initial stopped preparation; see
[RendererDeferred](RendererDeferred.md) for shared scene/replacement ownership.

## Initialization and job configuration

Run through the typed [OffscreenApplication](../app/OffscreenApplication.md).
Populate `DrawOffscreenApplicationConfig` in `initConfig`, including `frames` and
output extent. Initialization through either inherited init interface requires
its Engine's actual OffscreenSurface. Constructor and `setRenderer` strip overlays;
preparation also rejects renderers publishing overlays or lacking linear recording.
RendererDeferred implements `supportsLinearRecording`, `recordLinear`,
`recordFinal` and `sceneFingerprint` for this path.

The first `prepareFrame()` binds live renderer/scene settings and calls
`extendSettingsCatalog(Renderer&, SettingsCatalog&)` for application-owned IDs.
Defaults are live object values, then optional explicitly supplied persistence,
then an explicit graphics document, then an explicit fixed internal extent or
scale. Supplying a document bypasses persistence. Fixed internal dimensions disable
the scale policy; scale enables it. Output extent, frame count and backend remain
job settings. Invalid imports fail before scene submission; unknown IDs reject
unless explicit skip is requested. Inspect `settingsResult().issues` and report
non-error skip diagnostics. No implicit name matching maps generated light IDs
between applications. See [settings](Settings.md#headless-import-and-precedence).

## Samples and publication

`render` updates the renderer once and records one reconstructed linear sample.
Final internal completion hooks contribute it only after GPU completion. Exactly
`config.frames` contributions resolve through [FrameAccumulator](FrameAccumulator.md).
The final-only submission applies tone/color and optional final processing once.
`completedFrames()` reports completed scene contributions; `hasResult()` becomes
true only after final completion. Export in the last `didRenderFrame` callback,
using `_engine->offscreenSurface()` while device and surface are alive.

Do not override continuation to extend this job beyond its requested count.
`completeFrame`, `needsResolve`, `resolveFrame`, `completeResolve` and `finish` are
final overrides, so overriding `didRenderFrame` cannot bypass counting/publication.
Early termination or missing final completion rejects. Cleanup clears settings
bindings and sample owners before the inherited renderer cleanup. Count/result
accessors are not owners of the destroyed surface.

## Time, history and frozen state

Zero timestep freezes scene time; disable any frame-counter-based application
animation as well. Before sample zero, the delegate invalidates renderer/effect
and upscaler histories. Every requested sample contributes, including the first;
there are no discarded warm-up samples. Temporal effects and available FSR still
advance, so inputs can be correlated and history-filtered. Nonzero fixed timestep
explicitly averages an animation sequence.

Settings, renderer generation, scene-root identity and output extent remain fixed.
Frozen jobs also check the renderer's non-jittered scene/camera/material/light/history
fingerprint and reject detected mutation. Opaque in-place writes to existing GPU
allocations require caller serialization or explicit invalidation. Start a fresh
job after edits. Built-in final G-buffer debug views reject because they do not
consume the averaged reconstructed signal. Custom renderers/passes must honor the
same linear-input/final-only contract.
