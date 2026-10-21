# Deferred renderer and runtime ownership

Headers: `<bg2e/draw/Renderer.hpp>`, `<bg2e/draw/RendererDeferred.hpp>` and
`<bg2e/draw/DefaultRenderLoopDelegate.hpp>`. This is the experimental draw path;
production `bg2e::render` remains supported.

## Lifecycle

`DefaultRenderLoopDelegate` exclusively owns a `std::unique_ptr<Renderer>` and
retains a shared scene root. Its non-template `setRenderer(renderer, configuration)`
queues replacement; `setScene(root)` queues attachment. `prepareFrame()` commits
these changes before acquisition and UI recording, even while scene rendering is
paused. Scene identity survives renderer replacement. The renderer lifecycle is
`initialize` → `attachScene` → `update`/`record` → `cleanup`; resize and format
reconfiguration run at synchronized boundaries. Cleanup must tolerate partial init.
Stop background GPU producers throughout replacement, resource edits and teardown.
The Engine/device must outlive renderer, scene realizations and all shared passes.

`renderer()` is borrowed and can change at the next safe boundary. Delayed work
uses `rendererGeneration()` and `enqueueRendererUpdate(generation, operation, stale)`;
never capture the old renderer pointer. `withRenderer` checks a generation.
`setRendererChangeCallbacks` detaches borrowed targets before cleanup and attaches
them after initialization. `RenderSettingsPreferences::connect` installs those
callbacks; compose existing callbacks explicitly before using another owner.

`RendererConfiguration` carries output/internal extents, color/depth formats,
public layer selections, environment snapshot, soft-shadow pass, upscaler and
resolution settings. A zero internal extent follows output; an explicit extent is
fixed across output resize unless enabled resolution policy overrides it. The
actual output format/extent comes from the retained target. The default delegate
creates an empty scene; override `createScene()` or supply `setScene`.

## Rendering and materials

`RendererDeferred` extracts draw geometry, world transforms, camera, environment
and at most eight enabled lights. `setCamera(view, projection)` overrides the scene
camera; `useSceneCamera()` restores it. Frame data is slot-owned; scene time and
`sceneFrameNumber` advance with rendered scene work, independently of UI frames.
The graph uses single-sample G-buffer/depth, lighting, ordered transparency,
linear HDR reconstruction and one final tone/color operation. Six G-buffer
channels and preserved opaque depth support subsequent operations. Debug views
inspect the last processed layer; `setInspectOpaqueLayer(true)` isolates opaque
work. Debug output is not a whole-scene transparency composite.

Ray-tracing scene resources are renderer-owned: one collected instance list,
material/instance tables and TLAS per acquired frame, shared by shadows, AO/GI,
reflections and both composition layers. Shadow-specific query tables are built
from the same dense instance order. The renderer records the TLAS build before
the layers; consumers do not build independent scenes. Each slot retains its
snapshot until GPU completion. Immutable material/resource tables persist, and
completed matching TLAS/instance generations are reused across frames. Camera-only
movement skips acceleration builds after warmup; changed instance state uses new
immutable storage with a full build. `tracingResources()` exposes the shared service
for diagnostics (null when unavailable/uninitialized). At a synchronized update
boundary, `setAccelerationReuseEnabled(false)` restores per-snapshot full builds;
`setPersistenceEnabled(false)` independently rebuilds material/shadow generations.
Effect-specific uniforms, outputs and layout-dependent descriptor sets remain
separate. See [TracingSceneResources](TracingSceneResources.md) for table capacities
and standalone effect preparation.

`pipeline::SoftShadowPass` uses optional ray queries and nonuniform sampled arrays,
not a ray-tracing pipeline. Unsupported query/array hardware retains raster PBR/IBL.
Alpha-aware traversal uses up to 16384 albedo image/sampler pairs, clamped to device
budgets after reserving (2 images, 1 sampler, 9 total resources). Its v2 query kernel
uses Vulkan specialization ID0 or Metal argument-record buffers5/6. The reflected
alphaTextureLimit reports the selected alphaTextureCapacity(), not a fixed eight.
Overflow warns and omits only excess texture sampling, preserving scalar alpha/
cutoff, every occluder and the rest of the scene's shadows. It no longer clears
whole-scene visibility on the ninth pair. Transparent objects are excluded from default
occluders. Alpha cutoff zero preserves coverage; positive cutoff discards texels.
Configure every submesh: the standard cube has six face submeshes.

The delivered example's HDR precision failure has been corrected and confirmed
by the project lead. A separately reported transparency issue remains explicitly
deferred; transparency implementation is not a claim of complete runtime acceptance.
Public AO/GI/reflection tracing, effect histories, spatial filtering and manipulation
are available. [Integrated bake](../bake/index.md) now supplies completed RGB UV2
maps through [explicit material modes](BakedIndirectMaps.md). [Windowless draw frame execution](../app/OffscreenApplication.md#experimental-draw-execution)
and [exact camera-image averaging](DefaultOffscreenApplicationDelegate.md)
are available; standalone bake scheduling remains a separate product. See [advanced lighting](../../architecture/draw_advanced_lighting.md).

## Public extension contracts

`draw::pipeline::RenderLayer` is public. Its lifecycle includes `initialize`,
`attachScene`, `resize`, `update`, `record` and `cleanup`. A `LayerSelection`
retains shared ownership of the pass and configuration. A pass instance belongs
to one renderer/engine lifecycle. `Renderer::recordLayer` validates the contract
before virtual dispatch: shader-interface identity, ordered semantic slots,
formats, usages, image types, extent domains and device must agree. Merely matching
formats does not make a replacement shader compatible.

`PassContext` borrows frame, configuration, images and optional `RasterFrame` for
one recording call. Resources remain alive until submitted commands complete;
callers establish layouts/dependencies. Applications can replace compatible
GBuffer, deferred/final and skybox stages through configuration rather than edit
private renderer code. `pipeline::FinalLayer` accepts `reconstructedColor=true`
for output-domain HDR input; its default constructor mode remains internal-domain.
See the injected CountingGBuffer in the [tutorial](../../tutorials/draw/10_deferred_renderer.md).

See [resources](Resources.md), [environment](Environment.md) and
[settings/upscaling](Settings.md).

See [RTAmbientOcclusion](RTAmbientOcclusion.md) for the optional public scalar AO pass and shared tracing snapshots.

See [RTGlobalIllumination](RTGlobalIllumination.md) for the optional diffuse HDR GI pass and IBL/AO/GI mode selection.

See [RTReflections](RTReflections.md) for output-relative traced HDR specular radiance and confidence-based IBL composition.

See [TemporalAccumulator](TemporalAccumulator.md) for independent rendered-view effect histories, borrowed public surfaces, camera motion, bounded convergence and invalidateHistories application notifications.

See [DenoiseFilter](DenoiseFilter.md) for reusable scalar/HDR bilateral filtering after temporal accumulation, explicit surface guides, confidence semantics and live accessor metadata.

## Effect and final-processing configuration

RendererConfiguration appends shared ambientOcclusion, globalIllumination,
reflections, temporal, denoise and finalProcessor components; matching renderer
accessors return their actual live owners. With no explicit configuration, indirect
lighting selects GlobalIllumination and the renderer-created GI and reflection
passes are enabled. These are requested defaults: available/status and the selected
lighting graph still determine whether native effects can run. Unsupported effects
preserve the request and use the renderer's documented fallback. Injecting a pass
or setting its enabled flag/mode explicitly overrides the relevant default. Null
temporal creates enabled maxFrames=32; null denoise creates enabled radius=2. A null
finalProcessor adds no extra final processing.

Compatible components are initialized/cleaned with the renderer lifecycle; injected
shaderRoot/interface/prefix must match the native ABI. Callers can substitute
objects and metadata/settingsInstance without private-target access. Graph selection
accounts for compatible injected lighting contracts; usesAmbientOcclusion,
usesGlobalIllumination and usesReflections report graph support, not merely enabled.

Renderer supplies a shared BlueNoise atlas for available effects without injected
noise. Individual injected owners take precedence; missing atlas falls back to
hash sampling with noiseStatus. invalidateHistories resets effect generation and
SDK history; resetUpscaler remains a forwarding compatibility hook.

[FinalProcessor/SMAAProcessor](SMAAProcessor.md) runs only after current final color,
once per rendered frame, before overlays/UI. See [signal/quality/surface contracts](../../architecture/draw_advanced_lighting.md)
and [settings](Settings.md) for actual modes/diagnostic IDs.

## Public linear/final split

Renderer::supportsLinearRecording is optional and true for RendererDeferred.
recordLinear(frame) records the fully composed/reconstructed output-sized linear
RGBA16F image and returns its retained slot owner; complete submission before
readback/reuse. recordFinal(frame, linearImage) consumes that format on the same
updated slot/serial and records final color, optional FinalProcessor and overlays.
Ordinary record(frame) calls both, preserving normal per-frame behavior. Duplicate
linear/final recording rejects. sceneFingerprint() exposes a non-jittered scene,
camera/resource/history identity for frozen jobs; opaque in-place GPU writes need
explicit invalidation. The headless default excludes overlays and runs final
conversion once on the exact N-sample mean using
[pipeline::FrameAccumulator](FrameAccumulator.md).
Its reference implementation sums on the CPU in double precision and resolves
back to RGBA16F once. See [the default offscreen job contract](DefaultOffscreenApplicationDelegate.md)
and [accumulation architecture](../../architecture/draw_offscreen_rendering.md).

Temporal/denoise and effect/fullscreen/shadow invocation preparation reuse retired
[temporary resources](TemporaryResourcePool.md). Histories and returned signal
owners remain independent across in-flight slots and transparent layers. Controls
are available on the exposed temporal, denoise and effect components through
setPoolingEnabled(false) and poolStatistics(); change them at the normal synchronized
update boundary. Reuse does not change the existing temporal-invalidating signature.

The renderer reuses [temporal signature components](TemporalInvalidation.md) while
preserving the full serialized reset policy, including object motion. Use
setTemporalSignatureValidationEnabled(true) for exact reference parity and
setTemporalSignatureReuseEnabled(false) for full-serialization decisions. Both are
synchronized diagnostic controls. Out-of-band sceneFingerprint calls preserve the
old byte format and do not consume edits before update. No narrower effect reset
or additional dynamic-object history retention is enabled by this optimization.
