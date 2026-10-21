# OffscreenApplication

Headers: `<bg2e/app/OffscreenApplication.hpp>`,
`<bg2e/app/DrawOffscreenApplicationDelegate.hpp>`,
`<bg2e/app/OffscreenExecution.hpp>`. Namespace: `bg2e::app`.

`OffscreenApplication` owns a windowless application run. The delegate type passed
to `init(argc, argv, appId, sharedDelegate)` selects its graphics framework:

| Delegate | Execution mode | Graphics |
|---|---|---|
| `OffscreenApplicationDelegate` | `ProductionRender` | Production render/Vulkan |
| `DrawOffscreenApplicationDelegate` | `ExperimentalDraw` | Experimental draw over Vulkan or Metal |

`executionMode()` reports the selected mode. Production remains supported; passing
a draw delegate explicitly selects the new path. Neither route creates an
interactive MainLoop. The destructor releases initialized but unrun draw jobs;
errors during run, including cleanup, can propagate, while destructor cleanup
suppresses them.

## Production configuration and delegate

`OffscreenApplicationConfig` defaults to width=1920, height=1080, color/depth
creation enabled and `VK_FORMAT_R8G8B8A8_UNORM`. This production configuration
uses native Vulkan types. Its delegate receives `initConfig`, `init`,
`initFrameResources`, `initScene`, `resize`, then `frame`/`render` and
`didRenderFrame` after completion, followed by `cleanup`. The production
`render` callback returns true to continue and false to finish. Existing production
examples and [lightmap generator](../../apps/lightmap_generator/index.md) retain
this separate API.

## Experimental draw execution

`DrawOffscreenApplicationConfig` is defined with the typed delegate:

| Field | Default and contract |
|---|---|
| `engine` | `draw::EngineConfig`; Metal on macOS, Vulkan elsewhere; B8G8R8A8_UNORM and D32_SFLOAT by default |
| `extent` | 1920×1080 final output; nonzero |
| `createDepthImage` | true; false selects Undefined surface depth |
| `timeStepSeconds` | 0; finite, nonnegative; first scene delta is always zero |
| `frames` | 1, validated in 1..1048576; exact completed samples for the default accumulating delegate |
| `graphicsConfig` | Empty; optional settings-file path read before GPU allocation |
| `persistedDefaults` | Null; optional borrowed Preferences owner, which must outlive the job |
| `unknownSettings` | `draw::UnknownSettingPolicy::Reject`; explicit `SkipUnknownOrUnavailable` reports skipped properties |
| `internalExtentOverride` | None; nonzero fixed internal size |
| `scaleOverride` | None; finite scale in [0.25, 1], mutually exclusive with fixed internal size |
| `graphicsDocument` | Optional parsed JsonNode owner; populated from graphicsConfig when a path is supplied |

Output formats must be normalized/floating color; integer/depth color formats and
invalid requested depth formats reject. Native device support/allocation is still
required. `readPixelsRGBA8` specifically requires R8G8B8A8_UNORM, so the shipped
scene CLI explicitly requests that format instead of the EngineConfig default.

The app validates and parses configuration, calls `configure(config)`, creates the
[headless Engine](../draw/Engine.md#headless-initialization), then calls
`init(Engine*, OffscreenSurface*)`, `initScene`, `resize` and stopped
`prepareFrame`. No SDL video window, input or UI runtime is initialized.

For each scene frame the driver checks `continueRendering`, calls `prepareFrame`
and `update(seconds, sceneIndex)`, checks continuation again, acquires a surface
frame, and submits `render(FrameContext const&)`. After GPU completion it calls
`completeFrame(index)`. If `needsResolve()` is true, it submits `resolveFrame`
and calls `completeResolve` after that submission completes. Finally it ends the
surface frame and calls `didRenderFrame(index, elapsedMs)`. Helper/final submissions
do not increment the scene count. `finish()` runs when the loop ends.

The generic delegate leaves completion/resolve/finish hooks empty and owns its own
continuation policy. Its `frames` field alone does not implement an average.
Use [DefaultOffscreenApplicationDelegate](../draw/DefaultOffscreenApplicationDelegate.md)
for the exact-N scene-rendering path. Custom delegates record ordinary jobs through
the public GPU/FrameContext API; neither interface grants access to private renderer
targets.

## Completion and cleanup

Submissions are serial and synchronous. A failed submission produces no completion
notification. During `didRenderFrame`, completed surface color is TransferSrc and
can be read synchronously while Engine/device/surface are alive. FrameContext
references are borrowed during recording; retain or copy required host output
before run tears down its context. The default delegate exposes `hasResult()` only
in the last callback, after final GPU completion.

The app drains GPU work before delegate cleanup and Engine destruction. Cleanup
must tolerate partial initialization and release retained GPU owners while the
device is alive. The first initialization/run exception is preserved if cleanup
also throws. Reentrant init/run and init on an active app reject. Stop background
producers throughout initialization, resource edits and teardown.

See the [CLI guide](../../apps/draw_offscreen_application.md),
[tutorial](../../tutorials/draw/13_offscreen_application.md) and
[accumulation architecture](../../architecture/draw_offscreen_rendering.md).

## Standalone UV batches without camera frames

[draw_lightmap_generator](../../apps/draw_lightmap_generator.md) uses the typed
draw delegate with continueRendering false, a 1x1 depthless context surface and
finish to run StandaloneBakeBatch. No surface camera render callback is recorded;
UV surfaces use job resolution. Assembly/context/baker native owners are local
to that operation and release before driver cleanup. Config is validated against
actual bake settings before GPU initialization rather than camera graphics targets.
This does not change the default scene delegate's exact-N camera mean or production
overload. See [ownership architecture](../../architecture/draw_standalone_lightmaps.md).
