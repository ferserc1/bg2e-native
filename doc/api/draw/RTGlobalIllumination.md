# RTGlobalIllumination

`<bg2e/draw/pipeline/RTGlobalIllumination.hpp>` provides a public Device-backed diffuse GI pass, also included by draw/all.hpp. Production render remains separate.

```cpp
auto gi = std::make_shared<bg2e::draw::pipeline::RTGlobalIllumination>();
gi->setEnabled(true);
gi->setSampleCount(4);
gi->setBounceCount(2);
configuration.globalIllumination = gi;
configuration.indirectLighting = bg2e::draw::IndirectLightingMode::GlobalIllumination;
```

RendererDeferred supplies a disabled default when no component is injected. Configure an injected RTGIConfiguration with the deployed shaderRoot, optional shared utils::BlueNoise and compatible native shaderPrefix before initialization. Its interface is draw.rtgi.v1. Lifecycle/prepare/record/availability/outputExtent are virtual for compatible replacement. globalIllumination() exposes the component; usesGlobalIllumination() reports graph capability, independently of enabled() and the requested mode. setIndirectLightingMode selects IBL, AmbientOcclusion or GlobalIllumination, preserving both effects' settings. Mutate at synchronized application boundaries and request a scene frame; call invalidateHistories() after discontinuous application edits; the renderer coordinates its effect and SDK state.

Settings expose sampleCount (1..64), bounceCount (1..8), shadowSamples (1..64), positive finite bias/maxDistance, quality, useShadows and useBlueNoise. Active recording requires maxDistance > 2*bias. Quality reuses Low/Medium/High/Ultra scales 1/3, 1/2, 2/3 and 1, independently of renderer resolution. The renderer detects quality target changes.

Standalone clients initialize a Device/backend/slot count, prepare a retired slot with a RenderQueue or compatible TracingSnapshot, then record RTGIContext. Context supplies SurfaceInputs, output, LightUniforms and retained EnvironmentResources. Both camera-depth and explicit position/normal/coverage profiles use the [RTAO surface contract](RTAmbientOcclusion.md). Output must be sampled/storage/color-attachment RGBA16_SFLOAT at outputExtent(surface.extent()). RGB is linear incoming diffuse radiance normalized by PI, before receiver albedo; alpha one marks a valid estimate. Uncovered, disabled, unavailable or empty-scene output is zero with alpha zero. A valid black signal still has alpha one.

GI replaces the diffuse IBL term when valid. Specular IBL, sheen, receiver material AO, baked multiplier, direct light and emission remain in their existing composition. GI mode suppresses live RTAO without changing its requested settings. Invalid/unavailable GI restores the ordinary IBL/baked path. Actual baked-presence policy remains M04 work.

Vulkan uses iterative raygen transport with miss/closest-hit/any-hit and recursion depth one; Metal uses compute with nearest committed intersections. Both evaluate shared material tables at LOD0, albedo UV/cutout, scalar emission channels, geometric hit normals, participating lights and environment escape/terminal ambient. LightData.flags.w is the existing affectsReflections participation bit, matching the production GI light selection. Blended transmission and normal-map GI hit shading are outside this baseline. Available profiles require native execution, nonuniform material arrays and HDR storage support. status() records unavailable initialization diagnostics; requested settings remain intact.

Each prepared slot retains its snapshot and each invocation retains uniforms/descriptors/noise/environment through completion. Caller-owned surface/output images must also survive completion. A shared snapshot must match Device and selected 32/128/256 capacity; AO and GI may select different capacities because GI reserves an extra environment resource. reset/cleanup drain at stopped boundaries; call cleanup before destroying the Device. [Temporal histories](TemporalAccumulator.md) handle GI independently; [spatial filtering](DenoiseFilter.md) is available after temporal accumulation.

See [native ABI and implementation handoff](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/rtgi.md) and [manual fixture](../../../tests/draw/rtgi/README.md). Compilation and runtime validation belong to the project lead.

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
