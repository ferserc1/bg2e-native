# RTReflections

`<bg2e/draw/pipeline/RTReflections.hpp>` exposes the public draw reflection pass; draw/all.hpp includes it. This remains separate from production render.

```cpp
auto reflections = std::make_shared<bg2e::draw::pipeline::RTReflections>();
reflections->setEnabled(true);
reflections->setQuality(bg2e::draw::pipeline::RTReflectionQuality::High);
reflections->setMaxRoughness(0.75f);
configuration.reflections = reflections;
```

RendererDeferred creates an enabled default pass when none is supplied. Reflections are requested by default and remain independent of the GI/AO/IBL mode. Check available/status and usesReflections(); unsupported hardware or shaders preserve the request while the renderer falls back without traced reflections. An injected pass or explicit enabled setting overrides this default. Configure an injected RTReflectionConfiguration before initialization with deployed shaderRoot, optional shared BlueNoise and a compatible shaderPrefix/interface (`draw.rt_reflections.v1`). Lifecycle/preparation/recording, availability and outputExtent are virtual. RendererDeferred::reflections() exposes the component and usesReflections() reports graph support, independently of enabled(). Reflections coexist with IBL, RTAO or RTGI mode selection.

Settings expose enabled, quality, sampleCount (1..64), bias/maxDistance (finite positive), maxRoughness ([0,1]), roughnessSpread ([0,16]), shadowSamples (1..64), useShadows and useBlueNoise. Active recording requires maxDistance > 2*bias. Mirrors at roughness <= 0.01 use one deterministic ray; glossy surfaces increase toward sampleCount with roughness. Changes belong at synchronized application boundaries, followed by a scene-frame request; call invalidateHistories() after discontinuities to coordinate effect and SDK resets.

Quality is relative to display output, independently of internal renderer/G-buffer/FSR scale. Low/Medium/High/Ultra use 1/3, 1/2, 2/3 and 1 with nearest integer rounding and a minimum of one pixel. outputExtent(displayExtent) computes the required target. RendererDeferred detects reflection quality changes and rebuilds targets; an internal-scale change does not change the reflection dimensions when display/quality are fixed.

Standalone clients initialize(device,backend,slots), prepare a retired slot from RenderQueue or a compatible TracingSnapshot, then record RTReflectionContext. Context contains the existing camera-depth or explicit position/normal/coverage SurfaceInputs, receiver material image (sampled RGBA8 UNORM or RGBA16 float, G=roughness), displayExtent, cameraPosition, LightUniforms, retained EnvironmentResources and output. Receiver material matches the surface extent; output is sampled/storage/color-attachment RGBA16_SFLOAT at the display-relative extent. Receiver and output resolutions may differ. Keep caller-owned images alive through completion.

RGB contains the average outgoing linear HDR radiance at accepted geometry hits, before receiver Fresnel/BRDF weighting. Alpha is accepted finite geometry hits divided by rays actually sampled. RGB is not premultiplied by alpha. Misses, uncovered pixels, rejected roughness, empty scenes and disabled/unavailable passes return zero confidence. The lighting pass mixes traced radiance with prefiltered specular IBL using confidence, then applies the receiver's existing split-sum BRDF, material AO and baked multiplier. Diffuse GI/IBL, direct PBR, emission and sheen retain their composition. Black geometry hits can have confidence one; environment misses use IBL rather than a duplicate traced environment term.

Vulkan uses raygen/miss/closest-hit/any-hit at recursion depth one. Metal uses compute nearest-hit queries. Both reuse the shared bounded tracing tables and cutout predicate, evaluate albedo/emission and normal-map slots at explicit LOD0, and include environment diffuse plus participating direct lights at the hit. LightData.flags.w is the existing affectsReflections participation bit. Normal interpolation uses inverse transpose; tangents use the model transform, orthogonalization and mirrored determinant sign, matching raster's XYZ-only source tangent convention. The single source tangent basis also applies when a normal texture selects UV1; no extra tangent set is invented.

Snapshots must match the selected Device/capacity. Noise/environment/invocation resources remain retained in the slot through GPU completion. reset/cleanup drain at stopped boundaries; call cleanup before Device destruction. [Temporal histories](TemporalAccumulator.md) preserve reflection confidence independently; [spatial filtering](DenoiseFilter.md) is available after temporal accumulation.

See [implementation ABI and limitations](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/rt-reflections.md) and the [manual validation fixture](../../../tests/draw/rt_reflections/README.md). Compilation/runtime checks remain with the project lead.

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
