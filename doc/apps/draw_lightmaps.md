# draw_lightmaps

Source: [examples/draw/12_lightmaps](../../examples/draw/12_lightmaps/README.md).
Target: `draw_lightmaps`. The application ID is
`org.bg2e.examples.draw.lightmaps`. It runs through the experimental draw/bake
APIs and does not replace production editor/standalone bake applications.

The static room displays direct light, environment/IBL, emission and transparency.
Its floor and two opaque cubes are bake targets; one cube is an independent red
clone. Walls/emission remain unbaked, and transparent geometry is display-only.
This example explicitly selects IBL and disables live AO/GI and reflections for
comparison, overriding the renderer-wide GI/reflections defaults. The
example does not restore graphics preferences automatically, save files or accept
CLI options.

## Controls

1. While idle, edit the actual [LightmapSettings](../api/bake/LightmapSettings.md).
   Example defaults are GI, 128 square, 16 accumulated updates, 4 rays/update,
   two bounces and RGB32F CPU output. Library defaults are listed separately.
2. Select **Generate UV2 and bake / restart**. Fresh independent UV2 replacements
   are generated from original drawables; targets bake sequentially.
3. Read exact completed samples/target progress. **Cancel** drops partial maps;
   configure new values and start again. Settings editing is hidden while running.
4. Once every target completes, maps are applied together. Toggle **Apply baked
   indirect maps** for comparison. Direct light/emission and GI specular IBL
   remain active and do not receive a second diffuse multiplier.
5. Use **Next target**, **UV1** and **UV2** to inspect UV diagnostics and the
   latest completed multiplier. Atlas validation and resource rebinding follow
   geometry/resolution changes. The previews are linear diagnostic images, not
   gamma-corrected export previews.

One synchronous GPU sample runs before window acquisition on each update. Its
completion is confirmed before the baker counts it. UI remains available between
samples, but heavy work can delay input. Required unsupported RT profiles or
invalid output are reported without terminating ordinary scene display.

## Precision and ownership

The application captures typed CPU results before material adoption. Its direct
sampled GPU result remains RGBA32F, with a separate RGBA16F G-buffer multiplier
for display. Values outside [0,65504] are rejected for display, not silently
clamped. CPU RGB8 GI exposure/clamping is independent of the displayed map.
Only marked linear RGB UV2 maps use the AO slot as baked indirect lighting;
scalar AO and a separate lightmap cannot coexist in the current six-slot material.
GPU-only sampler metadata is not an exported resource or reloadable file.

The context freezes source/settings snapshots. Cancel/restart/mutation occurs
outside submitted work; previews retain image/sampler owners and are cleared
before UI/Engine teardown. See the [step-by-step tutorial](../tutorials/draw/12_lightmaps.md)
for actual sample/adoption/cleanup code.

## Build and resources

Build with the repository's configured platform generator and Vulkan SDK. Linux
uses Ninja, Windows Visual Studio 17 2022; macOS Xcode is required for native file
dialog support (Ninja remains suitable for development without those dialogs).
The normal examples configuration registers draw_lightmaps through bundle_app_sdl;
there are no additional example assets or private bake shaders.

Both paired native trees, `shaders/src/draw` and `shaders/src/bake`, are required.
They compile under the engine's paired-shader aggregate; app rebuilds refresh
both trees in resources even when only shaders change. RuntimeShaders installs
both directories. In particular `bake/uv_surface/surface.vert.metallib` must exist
inside macOS app shaders; Vulkan uses the corresponding `.spv`. A prior omitted
bake registration caused this file to be missing. Reload CMake and rebuild the
new target after updating that integration; use the newly rebuilt app bundle.

Use [packaging instructions](../../scripts/PACKAGING.md) for full runtime,
shader/assets and standalone-library distribution. A standalone project produced
from precompiled libraries is distinct from a headless bake scheduler.

## Verification

The [source example guide](../../examples/draw/12_lightmaps/README.md) supplies
Vulkan/Metal/no-RT checks, cancellation/restart/clone/resolution cases, both CPU
formats and scalar AO regressions. Lead continuation accepts the delivered
implementation/correction, but successful backend-specific numeric/timing/file
artifacts are not specified. The documentation agent performed static checks
only, with no build, shader compiler, app run or image export.
