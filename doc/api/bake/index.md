# Bake contracts

`<bg2e/bake/all.hpp>` exposes `bg2e::bake`, also exported by the root umbrella.
The namespace supplies CPU settings/pixel validation, Device-backed UV passes,
integrated execution and standalone scene/batch/output owners. The UI module provides
`BakeLightmapPreview` to display a completed `LightmapResult` through a draw UI
session while retaining its shared GPU image owner. The geometry UV diagnostic
is `bake::UvMapPreviewRenderer`; it rasterizes UV1/UV2 with submesh fills,
wireframes and atlas boundaries, then uploads the RGBA8 preview through
`gpu::Device` without modifying the mesh. Resolution changes or replacement
images must rebind the preview, and the widget must be cleared before UI/device
teardown.

LightmapSettings is the live settings object with accessors, validate(),
settingsType()/settingsInstance() (`bg2e.bake.lightmapSettings.v1`). Bind this actual
instance/owner to a SettingsCatalog. Scalar setters validate finite values/ranges;
validate() additionally checks selected distance/radius > 2*bias and dilation <=
resolution. Apply changes only at a stopped job boundary and invalidate/restart;
freeze a validated copy for execution, without a duplicate editable UI model.

```cpp
#include <bg2e/bake/all.hpp>

bg2e::bake::LightmapSettings settings;
settings.setMode(bg2e::bake::LightmapMode::RTGI);
settings.setResolution(512);
settings.setAccumulationFrames(16);
settings.setSamplesPerPixel(8);
settings.validate();

bg2e::bake::LightmapPixels pixels{
    1,1,bg2e::bake::LightmapPixelFormat::RGB32F,std::vector<float>{2,0,1}};
pixels.validate(); // Linear multiplier above one and valid black remain unchanged.
```

LightmapPixels is tightly packed linear RGB, row-major with top/UV-v=0 row first.
RGB8 is UNORM byte output; GI conversion applies exposureEV before clamp/round,
while RGB32F preserves finite nonnegative HDR. No gamma, alpha, premultiplication,
receiver albedo or implicit conversion/readback. validate() rejects invalid variants,
lengths, dimensions, overflow and float domain. The snippet creates CPU values only.

BakerContextConfiguration borrows a live Device and retains scene/environment and
optional public AO/GI/denoise owners and a replaceable UvSurfacePass owner (the
UV surface interface is described below). Injected mutable passes belong exclusively to
one bake lifecycle. No RendererDeferred or native Vulkan descriptor is required.
BakerContext and LightmapBaker remain abstract base interfaces. The concrete
[IntegratedBakerContext](IntegratedBakerContext.md) now provides preparation,
source/target generations, exact completed raw-sample progress and cancellation.
[Finished lightmap output](LightmapOutput.md) now publishes only after the final
completed submission passes strict numeric/count/coverage validation. Frame slots must be completed before reuse; Device
outlives all exported owners. Structural configuration validation is not a native
capability/profile check. Required unavailable effects must fail a concrete job.

LightmapResult declares an explicit UV2 RGB indirect multiplier and generations,
not a material AO-presence flag. AO is repeated visibility; GI is incoming diffuse
radiance/pi divided channelwise by unoccluded environment irradiance/pi (zero
reference gives zero). Uncovered/unbaked is neutral white. Covered invalid GI must
not silently become a successful map. CPU format is separate from actual GPU
floating image precision; reject HDR overflow rather than clipping float results.

The [contract and complete production mapping](../../../docs/render_to_draw/plan/milestone_04_lightmaps/contracts.md)
records defaults/ranges, settings-to-effect ownership, public world-surface formats,
required-capability/empty-scene policy and original implementation ownership.
Actual standalone availability is documented below; historical pending notes are
not an absence claim for delivered APIs.
See [draw surface inputs](../draw/SurfaceInputs.md), [tracing snapshots](../draw/TracingSceneResources.md)
and [manual CPU validation](../../../tests/bake/contracts/README.md). No agent build/runtime execution.

See [UvSurfacePass and safe UV2 reload](UvSurfacePass.md) for the implemented depthless world-surface, chart identity, coverage and target-only replacement contracts.

See [BakeLightmapPreview](BakeLightmapPreview.md) for the completed-map UI bridge
and [UvMapPreviewRenderer](UvMapPreviewRenderer.md) for the UV diagnostic API and
its ownership/lifecycle requirements.

The [integrated example](../../../examples/draw/12_lightmaps/README.md) drives
completed samples and adopts current results using [explicit baked material
metadata](../draw/BakedIndirectMaps.md). One synchronous submission per update
provides the example completion boundary; it does not add a render-loop callback.


## API and usage navigation

- [LightmapSettings](LightmapSettings.md): live metadata, exact defaults/ranges and frozen jobs.
- [Integrated context/target](IntegratedBakerContext.md): prepare/record/complete, generations and cancellation.
- [UV surfaces/reload](UvSurfacePass.md): device formats, geometry/chart ownership and safe UV2 replacement.
- [Lightmap output](LightmapOutput.md): public accumulator/composition, typed pixels and validation.
- [UV diagnostic](UvMapPreviewRenderer.md) and [draw UI bridge](BakeLightmapPreview.md).
- [Step-by-step tutorial](../../tutorials/draw/12_lightmaps.md) and [application/build guide](../../apps/draw_lightmaps.md).

- [Raw bake executor and sample](LightmapBakeExecutor.md): public effect reuse and exclusive pass ownership.
- [Integrated architecture](../../architecture/draw_integrated_lightmaps.md): data flow, precision and material boundaries.

## Standalone jobs and file publication

- [StandaloneBakerContext](StandaloneBakerContext.md) and [StandaloneLightmapBaker](StandaloneLightmapBaker.md): borrowed offscreen Engine, one live target, completed samples/generations/reset/cancel.
- [StandaloneBakeSceneAssembler](StandaloneBakeSceneAssembler.md): camera-less model/prefab/context, staged independent UV2 and retained Scene/root owners.
- [StandaloneBakeBatch](StandaloneBakeBatch.md): frozen serial jobs, full preflight, callbacks and complete commit.
- [LightmapOutputWriter](LightmapOutputWriter.md): protected sources, typed files, independent models and authoritative manifest; replaces the removed DB per-target writer.
- [Standalone C++ tutorial](../../tutorials/draw/standalone_lightmaps.md), [CLI/build guide](../../apps/draw_lightmap_generator.md) and [ownership architecture](../../architecture/draw_standalone_lightmaps.md).

The new product deliberately fails invalid required targets rather than skipping
into zero-success. RGB32F file encoding preserves dynamic range in standard Radiance HDR with RGBE quantization; raw GI
precision and interactive half-range material display remain separate limits.
Engine/Device outlive every retained GPU owner. Source/optimization preservation
and native evidence are distinct: ten synthetic inspector tests do not establish
actual Vulkan/Metal, geometry reload, package or performance results.

The canonical root `<bg2e/all.hpp>` now exports bake/all.hpp. Explicit
bake/all.hpp remains available. See [shared API ownership](../../architecture/draw_shared_api_retirement.md).
