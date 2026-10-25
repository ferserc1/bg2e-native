# Renderer and component injection

Header: `<bg2e/draw/Renderer.hpp>`. Namespace: `bg2e::draw`.
DefaultRenderLoopDelegate exclusively owns its Renderer through unique ownership.
RendererConfiguration retains shared component owners. Mutating components shared
between active renderer/bake lifecycles is unsupported; use separate instances.

## Configuration

| Field | Contract |
|---|---|
| outputExtent/internalExtent | Nonzero resolved single-sample extents; internal size can differ from output. |
| outputFormat/depthFormat | Default R8G8B8A8_UNORM / D32_SFLOAT; validate structural compatibility and actual device support. |
| layers | Ordered LayerSelection objects, compatible extents and shader contracts. |
| environment | Shared immutable EnvironmentResources snapshot. |
| softShadows | Null selects the default public query shadow pass. |
| ambientOcclusion | Null selects the disabled public AO default. |
| globalIllumination/reflections | Null selects enabled public defaults; indirectLighting defaults GlobalIllumination. |
| upscaler/resolution | Deferred defaults retain a disabled backend temporal implementation and create resolution settings; method/mode initialize the effective internal scale. Bilinear is the fallback. |
| upscaleMethod/upscaleMode | Optional None/FSR/MetalFX selection and Native/Quality/Balanced/Performance/UltraPerformance mode; see [Upscaler](Upscaler.md). |
| temporal/denoise/finalProcessor | Shared replaceable effect owners; final processor follows final color and is skipped when temporal reconstruction succeeds. Overlays follow. |
| overlays | Ordered non-null post-final passes; one lifecycle slot per mutable pass. |

Null behavior is specific to each slot, not a universal disabled setting.
validateConfiguration checks extents/contracts, not complete native support.

## Lifecycle and replacement

Implement configuration(), initialize(Engine&, RendererConfiguration),
attachScene(shared_ptr<Node>), resize(outputExtent, internalExtent), update,
record and cleanup. prepareFrame executes at the stopped boundary before surface
acquisition. Cleanup must tolerate partial initialization and release scene/engine
references. Surface format changes clean up/reinitialize the same renderer owner;
resize updates configuration and dependent resources. Reattachment resets
scene-dependent state. Renderer update owns traversal/animation policy.

FrameContext bindings are borrowed. Per-slot resource completion is required
before writing; do not retain frame references. Scene frame numbers and time
are separate from UI/presentation cadence. invalidateHistories marks camera cuts
and in-place resource edits. sceneFingerprint supplies frozen scene identity for
offscreen jobs; the base rejects unsupported identity queries. Optional
supportsLinearRecording/recordLinear/recordFinal hooks expose linear output to
camera accumulation before final conversion. `recordLinear` returns an
output-sized, single-level RGBA16F image with Sampled/TransferSrc/TransferDst
usage; preserve its contents until completion and stopped slot reuse.
`recordFinal` consumes that linear format on the same updated slot without
repeating scene work. The base throws for unsupported linear/final operations.

## Compatible extension points

Inject public layer, shadow, AO/GI/reflection, temporal, denoise, final, overlay or
upscaler objects through the configuration. Compatible subclasses/custom shader
libraries must satisfy declared formats, bindings, interface versions and native
stage conventions. Reflection metadata points to the complete dynamic settings
object, including multiple inheritance. Keep its owner alive and change live
configuration only at a synchronized boundary. Settings accessors read the actual
component; do not mirror them in a second editable model.

See [deferred renderer](RendererDeferred.md), [live settings/upscaling](Settings.md),
[shader libraries](../gpu/ShaderLibraries.md) and
[the runnable deferred example](../../../examples/draw/10_deferred_renderer/src/main.cpp).
