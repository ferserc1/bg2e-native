# RTAmbientOcclusion

`<bg2e/draw/pipeline/RTAmbientOcclusion.hpp>` exposes the draw AO pass; `<bg2e/draw/all.hpp>` includes it.

```cpp
auto ao = std::make_shared<bg2e::draw::pipeline::RTAmbientOcclusion>();
ao->setEnabled(true);
ao->setSampleCount(8);
ao->setQuality(bg2e::draw::pipeline::RTAOQuality::High);
rendererConfiguration.ambientOcclusion = ao;
```

Set RTAOConfiguration::shaderRoot to the deployed shader root when constructing a standalone pass. RendererDeferred supplies its shader root to the default pass. Configure injected passes before initialization. The default pass is disabled; available() reports capability/pipeline availability separately from enabled(). A custom subclass may override recording or a compatible shaderPrefix may replace the paired native kernels with interface `draw.rtao.v3`. The `_256` shader files are templates: Vulkan specializes constant ID0 to the exact device-clamped capacity up to 16384; Metal uses image/sampler argument-record buffers8/9. See [tracing tables](TracingSceneResources.md).

RendererDeferred prepares scene resources per retired frame slot and records camera AO for each composite. Its ambientOcclusion() accessor exposes settings. Change settings at a synchronized application boundary and request a scene frame. Quality affects AO resolution independently of renderer resolution. Low/Medium/High/Ultra scale by 1/3, 1/2, 2/3 and 1. Sample/bounce counts, radius, bias, falloff, attenuation and shared BlueNoise are configurable. Discontinuous edits require invalidateHistories() to reset engine and SDK histories; [Temporal histories](TemporalAccumulator.md) are available; [spatial filtering](DenoiseFilter.md) is available after temporal accumulation.

Standalone consumers call initialize(device,backend,slots), prepare(slot,serial,queue) or prepareSnapshot(...), then record(RTAOContext). Each serial is prepared once in a retired slot; renderedSample controls sampling independently of slot/presentation identity. Output is sampled/storage/color-attachment R16_SFLOAT. CameraDepth inputs contain encoded RGBA16 normals, D16/D32 depth and inverse view-projection. WorldPositionCoverage inputs contain signed RGBA16 normals, RGBA32 world positions and R8 coverage. All inputs share an extent/device; outputExtent() determines the output size. Borrowed images must remain alive through completion. Disabled/empty scenes return scalar one. Unsupported devices use the renderer's original lighting path.

TracingSceneResources builds reusable immutable geometry/material snapshots with dense TLAS IDs, canonical identity, both UV sets and independent bounded image/sampler tables. It retains source owners; consumers share snapshots instead of changing an in-flight slot. Call cleanup before Device destruction; reset/cleanup drain outstanding work.

See the [binding/ownership contract and limitations](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/rtao.md) and [manual validation fixture](../../../tests/draw/rtao/README.md). Implementation has been accepted by project lead continuation; exact device/scenario coverage is unspecified. No agent compilation/runtime execution is claimed.

RendererDeferred now coordinates effect and SDK resets through invalidateHistories(); resetUpscaler() forwards to it for existing adapters. See [temporal invalidation and ownership](TemporalAccumulator.md).

RendererDeferred applies the optional public [DenoiseFilter](DenoiseFilter.md) after temporal resolution and before lighting; filtered output is separate from temporal history.

## Live controls and current use

Both existing examples expose this component through the live [settings catalog](Settings.md).
Compatible replacement metadata must return its matching settingsInstance address.
Requested enabled/settings remain serializable independently of availability/status;
check graph selection as well. [SurfaceInputs](SurfaceInputs.md) and
[TracingSceneResources](TracingSceneResources.md) describe reusable inputs/owners.
Device must outlive retained results/snapshots. No UI, exact bake-average or
headless application scheduler is implicitly created by a standalone effect.

Invocation uniforms and surface set0 use [retired temporary storage](TemporaryResourcePool.md)
keyed by slot/invocation ordinal, layout and uniform size. Each live invocation
keeps distinct storage; only retired unreferenced leases are reused. Exact native
binding tuples skip descriptor updates, while uniforms are uploaded each call.
Tracing scene set1 and immutable material arrays set2 retain their existing owners.
Use setPoolingEnabled(false) for fresh-invocation diagnostics and poolStatistics()
for current retention, at a synchronized update boundary.
