# Deferred scene with live settings

The complete caller is [main.cpp](../../../examples/draw/10_deferred_renderer/src/main.cpp),
with [validation notes](../../../examples/draw/10_deferred_renderer/README.md).
Target/executable: `draw_deferred_renderer`. It uses Application/MainLoop and
experimental draw on Vulkan or Metal; production example 17 remains separate.

## Construct a renderer delegate

The following setup fragment creates a scene delegate; register input and UI
delegates as in [the quick start](../../api/draw/quick_start.md) before running.

```cpp
#include <bg2e/draw/all.hpp>

bg2e::draw::RendererConfiguration configuration;
configuration.resolution = std::make_shared<bg2e::draw::ResolutionSettings>();
configuration.resolution->setEnabled(true);
configuration.resolution->setScale(0.67f);
auto delegate = std::make_shared<bg2e::draw::DefaultRenderLoopDelegate>(
    std::make_unique<bg2e::draw::RendererDeferred>(), configuration);
application.setRenderDelegate(delegate);
```

Supply a shared scene via `setScene` or override `createScene` to load geometry
when Engine is available. The example creates a textured two-submesh OBJ,
nested opaque original/independent tinted clone, ordinary/solid glass, floor,
HDR environment, perspective camera and four rotating lights. Scalar material
controls demonstrate live attributes; clone guards compare CPU and GPU owners.
Packed-channel/UV1 and a checker canopy provide material/query probes. Uniform
cube fixtures use one full-buffer submesh so one material applies to every face.

The example also injects a public CountingGBuffer and an application-defined
Upscaler with its own live metadata. Renderer replacement queues safely while
retaining scene identity; a stale exposure-edit probe verifies generation rejection.

## Run and inspect

After the project lead builds the registered target and paired shaders, run the
platform executable. On macOS it is inside
`bin/macos/draw_deferred_renderer.app/Contents/MacOS/`; elsewhere use `bin/<platform>`.
Supported options:

```text
draw_deferred_renderer --backend=vulkan --upscaler=bilinear
--backend=metal
--upscaler=fsr
--upscaler=application
--no-rt
--validation
--settings=FILE
--restore-preferences
```

Metal is macOS-only. Defaults choose Metal on macOS and Vulkan elsewhere.
`--settings=FILE` bypasses requested stored defaults; neither restore path runs
without request. Select the matching upscaler before importing its settings.
The settings window exports/imports strict typed settings, exposes live availability
and reports rejected stale/type/unknown values.

Turn off transparent models and animation to inspect opaque material channels,
cutout holes and shadows. Toggle cutoff/shadows and try `--no-rt`: raster PBR/IBL
must remain. The forced option disables query intent, not device capability.
Debug views normally display the last processed layer. Query traversal rejects
more than eight distinct cutout texture/sampler pairs explicitly.

Try 25/67/100 percent scale, odd resizes, pause/resume and renderer/upscaler
replacement. UI remains live while scene time is paused. FSR has camera-only
motion and can ghost on moving objects/glass; unavailable SDK/backend falls back
to bilinear while retaining reduction. A separate transparency issue is deferred;
HDR overflow was corrected and confirmed by the project lead.

## Build and resources

Use Ninja on Linux, Xcode on macOS when native file dialogs are needed, and
Visual Studio 17 2022 on Windows. VULKAN_SDK is the required SDK input. Paired
sources live under `shaders/src/draw/` (GLSL and Metal in each pass directory),
with deployed shaders/assets under the platform runtime tree/bundle. Keep engine
and application shader/resource deployment together; missing paired shaders are
not an optional-feature fallback. See [build/distribution](../../apps/draw_deferred_renderer.md).

The agent checked this documentation statically, without compiling snippets or
running the example. The linked validation checklist distinguishes required
checks from lead-reported results. No advanced RT, manipulation, bake or offscreen
draw workflow is claimed by this tutorial.
