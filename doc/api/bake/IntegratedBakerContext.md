# Integrated bake execution

Include `<bg2e/bake/IntegratedBakerContext.hpp>` or `<bg2e/bake/all.hpp>`.
`IntegratedBakerContext` borrows an initialized `draw::Engine`; its configuration
must specify that Engine's Device/backend, frame slots, a loaded draw scene root
and, for GI, explicit retained EnvironmentResources. An empty shader root resolves
the deployed engine shader directory. It creates no
renderer or display history. The Engine/Device outlive every context, baker,
exported raw sample and GPU owner.

## Delivered boundary

This implementation records raw UV-space AO/GI, accumulates exact valid update
counts, composes the RGB multiplier and dilates chart padding. After the last
submission completes, strict validation/readback publishes Complete/result/pixels.
Recording alone never publishes output. See [output semantics](LightmapOutput.md)
for normalization, saturation rejection, chart isolation and typed conversion.

One context supports one live baker and one open/prepared/in-flight bake frame.
It supports the configured presentation slot indices, but serializes its own bake
samples. Other presentation work may continue independently; final validation/readback is
synchronous and can drain other Device submissions. Release the previous
baker handle before creating another target, or use separately owned contexts.
All methods are single-threaded. The service itself owns no viewport UI;
[draw_lightmaps](../../apps/draw_lightmaps.md) supplies a concrete example scheduler
and controls.

## Stopped preparation and sample lifecycle

Construct the context and call `createBaker(LightmapBakerConfiguration)` at a
stopped producer boundary. The actual live settings owner is retained; reset
freezes a validated copy. `frozenSettings()` exposes the copy used for execution.
CPU UV preparation uses the actual uploaded target mesh; target must remain
loaded, enabled and descended from configuration.sceneRoot.

```cpp
bg2e::bake::BakerContextConfiguration config;
config.device = engine.device();
config.backend = engine.backendType();
config.frameSlots = acquiredFrameSlotCount;
config.sceneRoot = sceneRoot;
config.environment = environmentResources;
config.shaderRoot = shaderRoot;
bg2e::bake::IntegratedBakerContext context(engine,config);
bg2e::bake::LightmapBakerConfiguration targetConfig;
targetConfig.target = targetNode;
auto baker = context.createBaker(targetConfig);

// During a completed acquired slot's scene command recording, outside scopes:
bg2e::bake::BakeFrame bakeFrame{
    command,slot,serial,context.sourceGeneration()};
context.prepareFrame(bakeFrame);
baker->record(bakeFrame);
// Application ends/submits command together with its normal presentation work.
// Only when the submission's GPU fence is confirmed complete:
context.completeFrame(slot,serial);
auto sample = baker->latestSample(); // Completed raw sample, not LightmapResult.
```

`prepareFrame(const draw::FrameContext&)` forwards commandBuffer, frameSlot,
frameNumber and the current source generation, checking Engine identity. Frame
serials must strictly increase across all slots, including abandoned/failed
preparations; sample indices are independent zero-based completed sample counts.
The command must be recording on the same Device and outside scopes. The generic
CommandBuffer API cannot inspect recording/device identity, so these remain caller
preconditions. The caller also establishes completed acquired-slot discipline.

Preparing twice, recording before prepare, a different command/slot/serial/source
job, duplicate record, an out-of-range slot or sampling beyond the requested count
throws. There is at most one attempted sample per prepared frame. Merely recording
never increments completedSamples or exposes the pending sample. `completeFrame`
requires the matching open slot/serial, checks current scene/target again, and
commits only a successful current uncancelled accumulation. On the final update it
validates sums/counts/coverage and the padded full-float output before publishing
the LightmapResult and cached typed CPU pixels. Repeating completion
throws. Preparing/completing a frame without a record contributes no sample. After Complete,
prepare rejects further samples until reset.

`abandonFrame(slot,serial)` is only for an unsubmitted command already discarded
or reset by its owner. It contributes no sample. A partial recording failure must
be discarded, abandoned and reset before sampling again; never submit that partial
command. A successful discarded sample can be retried with a new serial/index key.
Submitted work must retire via completeFrame after completion, including cancelled
work; it cannot use abandonFrame. The context cannot infer fence/submission state.

## Snapshot isolation and effect replacement

`LightmapBakeExecutor` is the reusable Device recording primitive. It retains a
UV pass, the selected AO or GI pass, per-slot snapshots and invocation outputs.
The selected public pass's TracingSceneResources creates bake-owned TLAS/material/
geometry bindings with its exact capacity; `prepareSnapshot` feeds that same
snapshot to the pass. It is built only within its command recording/submission,
never reused for another command; aborted snapshots are discarded. Raster,
selection, renderer TLAS/descriptors and view histories are never borrowed.

The selected pass uses Ultra/full target extent, enabled, samplesPerPixel, rayBias
and the selected radius/distance. AO uses one visibility bounce; GI uses giBounces.
The raw sequence key is completedSamples, not presentation slot/serial. Public
`draw/rtao/world_{hash,blue}_{capacity}` and
`draw/rtgi/world_{hash,blue}_{capacity}` native variants already implement the
required explicit signed normal/world position/R8 coverage profile; no duplicate
bake shading or wrapper shader family is introduced. Injected noise is retained by
the selected effect; absent/unready noise uses its bound-resource-free hash path.

Compatible application UV/AO/GI subclasses can be injected through passes. They
must start clean, initialize against this lifecycle, support the public native
interface, retain partial-record owners and remain exclusive/unchanged for the job.
Do not inject a display renderer's live mutable pass. The executor checks selected
required availability, matching-capacity snapshot, output format/usage and device
image limits; missing native support/profile/capacity fails rather than producing a
successful neutral fallback. GI rejects empty traceable snapshots. An empty AO
scene uses the public neutral visibility estimator. Configured denoise is rejected
until chart-isolated filtering exists; raw samples are never denoised here.

BakeSample retains the UvSurfaceResult, signal, snapshot, environment, lights,
mode and sampleIndex. AO signal is sampled/storage/readback R16 float visibility;
GI is RGBA16 incoming diffuse radiance/pi with alpha validity. It is not normalized
against irradiance, multiplied by receiver albedo, tone mapped or clamped. Chart
IDs and coverage remain in sample.surface for exact accumulation.
The output stage diagnoses invalid/saturated raw estimates and nonfinite/negative
or overflowing sums/multipliers before publishing final output.

## Edits, cancellation and lifetimes

The context compares fresh draw extraction with its captured baseline before
prepare/record/completion: node membership, mesh/tracing/material/image/sampler
identity, transforms, packed material/light values and logical environment intent.
Target checks include root ancestry, disabled ancestors, Drawable identity and
uploaded mesh identity. Removed/reloaded/edited targets or scene snapshots reject
sampling/publication. CPU edits and uploads must never race submitted work.

In-place image/GPU contents, CPU mesh edits without reload and opaque external
resource edits cannot be detected by pointer/value comparison. Stop producers,
retire submitted frames or discard/abandon unsubmitted ones, call
`invalidateSources()` and `baker->resetAccumulation()` after applying a safe edit.
Use the UV2 safe-reload beforeChange hook for generation invalidation and the
completion hook to restart against the adopted drawable. Settings-only edits use
reset at the same stopped boundary. Reset alone does not accept a changed scene
baseline: explicit source invalidation is required first.

Environment resource structs are copied while retaining their image/sampler
owners, so replacing the application's resource fields cannot drop a submitted
sample's images. Environment images remain immutable until retirement. To adopt
new environment owners, call `setEnvironmentResources(snapshot)` at the stopped
boundary, then reset; this updates context generation and rebuilds the executor
configuration without generating a second renderer environment. In-place image
updates still require source invalidation and all producers stopped.

`cancel()` hides completed/raw results immediately and retains pending owners;
completion after cancellation contributes no sample. `resetAccumulation()` drains
at a stopped boundary, advances target generation, freezes current live settings
and creates fresh UV/effect resources. Source invalidation advances the source
generation and requires reset. `latestSample()` exposes the completed raw sample in Running/Complete and hides
cancelled/invalidated/failed jobs; callers retaining a previous sample retain its GPU owners independently.

Explicit cleanup requires no open frame and drains submitted users. Destroying a
target handle does not release the context's pending owners. Context destruction
also drains submitted work, but unsubmitted commands must already have been
reset/discarded; destruction cannot reset the application's command buffer. Avoid
closing the Engine before all retained samples are released. sceneRoot must exclude
editor gizmo drawables or disable their tracing; the baker does not traverse UI,
selection overlays or private renderer queues.

See the [manual lifecycle fixture](../../../tests/bake/integrated/README.md),
[UV surface contract](UvSurfacePass.md), and
[M04-03 implementation handoff](../../../docs/render_to_draw/plan/milestone_04_lightmaps/integrated-context.md).
