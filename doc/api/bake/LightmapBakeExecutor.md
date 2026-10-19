# LightmapBakeExecutor and BakeSample

**Header:** `<bg2e/bake/LightmapBakeExecutor.hpp>` · **Namespace:** `bg2e::bake`

This public Device-backed primitive records one raw UV-space AO/GI estimate.
[IntegratedBakerContext](IntegratedBakerContext.md) supplies the surrounding job,
completion and publication policy. Use the integrated context for ordinary jobs;
use the executor when supplying your own scheduler over the same public passes.

## Lifecycle and methods

| Method | Contract |
|---|---|
| LightmapBakeExecutor(BakerContextConfiguration) | Validate structural configuration; borrow Device, retain scene/pass owners and copy the environment resource struct with shared image/sampler owners. |
| initialize(const LightmapSettings&) | Validate and freeze settings; initialize the selected required public AO/GI effect and UV surface pass. Call at a stopped boundary. |
| prepareGeometry(std::shared_ptr<draw::Drawable>) | Return retained immutable UvSurfaceGeometry for a loaded UV2 target. |
| prepareFrame(uint32_t slot, uint64_t serial, const draw::RenderQueue&) | On a completed slot, build the selected effect's matching-capacity TracingSnapshot and prepare UV/effect slots. Serials increase on slot reuse. |
| record(const UvSurfaceInput&, uint32_t sampleIndex, std::shared_ptr<draw::pipeline::TracingSnapshot>, const draw::LightUniforms&) | Outside command scopes, record one attempt for the prepared slot/serial and matching snapshot. Return retained raw signal/surface owners; do not submit. |
| settings() const | Read the frozen settings copy. |
| cleanup() | Drain Device work, clean exclusive passes and release slots before Device destruction. Required before reinitialization. |

`sampleIndex` is the surrounding scheduler's zero-based completed-update index.
A failed or partial command cannot be retried in place. The scheduler must discard
unsubmitted partial work or retire submitted work, then establish a fresh serial.
Shared pointers preserve resources; they do not prove submission completion.

## Signals and required capabilities

`BakeSample` retains its UvSurfaceResult, signal image, tracing snapshot, environment,
lights, sample index and mode. AO is R16_SFLOAT visibility; GI is RGBA16_SFLOAT
incoming diffuse radiance/pi with alpha validity. These are raw estimates, not
material-ready multipliers or completed LightmapResults. Valid GI black still has
alpha one. See [output validation](LightmapOutput.md) for saturation rejection,
exact counts, normalization and chart-safe padding.

The executor forces the selected effect enabled and full resolution (Ultra), with
the frozen ray count, bias, radius/distance and GI bounce count. External mutation
of those settings during recording is rejected. Required formats/usages, image
limits, native RT profile and material-table capacities must be supported. GI
requires valid explicit environment resources and a nonempty traceable snapshot.
Unavailable profiles fail the job rather than producing neutral success.

## Injection and ownership

BakerContextConfiguration::passes accepts exclusive UvSurfacePass and public
`draw::pipeline::RTAmbientOcclusion` / `RTGlobalIllumination` owners. Null selects
bake-owned defaults. Never share their mutable slots with a display renderer.
Shader roots and effect configurations must match their public interfaces and
native capacities. This reuses draw's world-surface kernels and material tracing;
it does not instantiate another camera renderer or private estimator.

The executor rejects a configured DenoiseFilter because that profile cannot
isolate atlas charts. Accumulator/composition injection belongs to the surrounding
integrated context, not this raw recorder. Keep geometry, scene, environment,
Device and invocation owners valid through completion. Stop producers before
initialization, source replacement and cleanup.

Sources: [declarations](../../../lib/include/bg2e/bake/LightmapBakeExecutor.hpp),
[implementation](../../../lib/src/bg2e/bake/LightmapBakeExecutor.cpp),
[UV surface contract](UvSurfacePass.md), [integrated caller](IntegratedBakerContext.md).
