# Deferred renderer and runtime ownership

Headers: `<bg2e/draw/Renderer.hpp>`, `<bg2e/draw/RendererDeferred.hpp>` and
`<bg2e/draw/DefaultRenderLoopDelegate.hpp>`. This is the draw path;
draw is the production renderer.

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
fixed across output resize unless enabled resolution policy overrides it.
Current deferred initialization enables that policy and derives scale from
`upscaleMethod`/`upscaleMode` on Vulkan and Metal; see [Upscaler](Upscaler.md). The
actual output format/extent comes from the retained target. The default delegate
creates an empty scene; override `createScene()` or supply `setScene`.

## Rendering and materials

`RendererDeferred` extracts draw geometry, world transforms, camera, environment
and at most eight enabled lights. `setCamera(view, projection)` overrides the scene
camera; `useSceneCamera()` restores it. Frame data is slot-owned; scene time and
`sceneFrameNumber` advance with rendered scene work, independently of UI frames.
The graph uses single-sample G-buffer/depth, lighting, ordered transparency,
linear HDR reconstruction and one final tone/color operation. Six G-buffer
color attachments and preserved opaque depth support subsequent operations. Debug views
inspect the last processed layer; `setInspectOpaqueLayer(true)` isolates opaque
work. Debug output is not a whole-scene transparency composite.

## Transparent layer

Transparent and solid-transparent submeshes are merged and stably sorted far to
near using the projected depth of their uploaded bounds center (origin fallback).
This is submesh sorting, not per-triangle order-independent transparency;
intersecting or nonconvex surfaces can still require geometry splitting.

The renderer uses two existing depth images with different roles:

- Preserved opaque depth contains only opaque geometry. Transparent work never
  clears or writes it; skybox visibility and HDR reconstruction also use it.
- Original G-buffer depth is reusable receiver scratch. Before the transparent
  batch and before each individual composition item, the renderer copies preserved
  opaque depth into this image. Receiver capture writes the actual transparent
  surface depth to scratch, rejecting geometry behind opaque surfaces.

Opaque depth alone cannot locate a transparent surface: over empty background it
contains the far-depth clear value. Using it to reconstruct transparent positions
produces background positions and incorrect lighting/effect receivers. Scratch
retains the configured depth precision; no additional depth images are allocated.
The copies add bandwidth proportional to the number of composition items.

Transparent recording has two stages:

1. Rasterize all visible receivers into the shared G-buffer with
   `RasterFrame::transparentItems` and `captureReceiverDepth=true`. The depth test
   selects the nearest surviving transparent receiver at each pixel. Compute
   shadows, AO/GI as configured, reflections, temporal accumulation and denoising
   once for this complete receiver layer, using its material/normal and actual
   scratch depth. Consume transparent temporal metadata/diagnostics at this point,
   before scratch is reused.
2. Replay individual submeshes in the same far-to-near order. Restore scratch from
   preserved opaque depth before each item so earlier transparent objects cannot
   occlude later items. Lighting reconstructs that item's actual position and
   samples the already resolved shared effects. Compose alpha and screen-space
   refraction against the accumulated HDR background, alternating the two existing
   color targets. A regional draw first copies the full accumulated background
   into its destination and loads it to preserve pixels outside the write region.

G-buffer material clears prepare receiver inputs; they do not clear the accumulated
HDR color. There is no independent GI/reflection history for each transparent
object. Effects represent the topmost receiver and are reused by surfaces behind
it; this is an intentional approximation, not independent indirect lighting for
every overlapping surface or recursive transparent light transport.

There is one opaque history domain (0) and at most one transparent history domain
(`RendererDeferred::TransparentTemporalLayer`, 1) per signal/view. Metadata replay
also records one shared transparent set. Raw effect targets are reused in sequence
within each acquired slot; histories and filtered owners remain separate between
opaque and transparent domains and across in-flight work as required. At fixed
resolution and quality, adding transparent submeshes does not allocate screen-sized
effect histories or depth images per object. Draws, resource bindings and depth
copies still grow with visible object count; geometry/tracing storage remains
scene dependent.

Custom G-buffer layers must honor both `RasterFrame::transparentItems` and
`captureReceiverDepth`, in addition to opaque and single-item selections. Direct
GBuffer transparent callers that leave `captureReceiverDepth=false` retain the
read-only depth pipeline; that path does not generate a transparent position guide.
See [transparent regions](TransparentRegions.md) for coverage and fallback rules.

## Shared tracing resources

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
budgets after reserving (2 images, 1 sampler, 10 total resources). Its v5 query kernel
uses Vulkan specialization ID0 with alpha arrays16/24 in persistent set2,
scene data in set1 and invocation inputs in set0, or Metal argument-record buffers5/6. The reflected
alphaTextureLimit reports the selected alphaTextureCapacity(), not a fixed eight.
Overflow warns and omits only excess texture sampling, preserving scalar alpha/
cutoff, every occluder and the rest of the scene's shadows. It no longer clears
whole-scene visibility on the ninth pair. Transparent objects are excluded from default
occluders. Alpha cutoff zero preserves coverage; positive cutoff discards texels.
Configure every submesh: the standard cube has six face submeshes.

The delivered example's HDR precision failure has been corrected and confirmed
by the project lead. The project lead also confirmed restored transparent
visibility after the receiver-depth correction. This observation does not establish
complete Vulkan/Metal parity or acceptance of every transparency scenario.
Public AO/GI/reflection tracing, effect histories, spatial filtering and manipulation
are available. [Integrated bake](../bake/index.md) now supplies completed RGB UV2
maps through [explicit material modes](BakedIndirectMaps.md). [Windowless draw frame execution](../app/OffscreenApplication.md#draw-execution)
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
once per rendered frame when temporal reconstruction was not applied, before
overlays/UI. Successful FSR/MetalFX reconstruction bypasses it; final color
conversion itself still runs. See [signal/quality/surface contracts](../../architecture/draw_advanced_lighting.md)
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
owners remain independent across in-flight slots and the opaque/transparent domains. Controls
are available on the exposed temporal, denoise and effect components through
setPoolingEnabled(false) and poolStatistics(); change them at the normal synchronized
update boundary. Reuse does not change the existing temporal-invalidating signature.

The renderer reuses [temporal signature components](TemporalInvalidation.md) while
preserving the current reduced history-policy comparison. Full fingerprints
remain transform-sensitive, while transform-only observation preserves global
histories and supplies scene motion to temporal scheduling. Use
setTemporalSignatureValidationEnabled(true) for exact reference parity and
setTemporalSignatureReuseEnabled(false) for full-serialization decisions. Both are
synchronized diagnostic controls. Out-of-band sceneFingerprint calls preserve the
old byte format and do not consume edits before update. Signature caching itself
does not authorize otherwise incompatible history reuse;
receiver compatibility and temporal capabilities are checked separately.

## Recorded camera coordinates (P2.1)

Views are right handed and engine clip depth is zero to one. Normalized framebuffer
coordinates are `(0.5 * clip.x / clip.w + 0.5, 0.5 - 0.5 * clip.y / clip.w)`;
framebuffer Y opposes clip Y (negative-height Vulkan viewport, matching Metal).
Motion is **previous minus current**, in pixels at the internal receiver extent,
using unjittered projections: `(previousNdc.xy - currentNdc.xy) * (0.5, -0.5) * extent`.
Raster coverage uses the current jittered projection. Previous history lookup will
use the previous jittered projection; jitter must not become object motion.

The private immutable correspondence snapshot stores view, unjittered projection,
jittered projection and internal extent for current and previous scene recordings,
as well as captured current/previous instance world matrices. Previous state must
belong to the consecutive scene sequence in the same history generation and an
already submitted recording in the renderer's ordered graphics stream. It need not
be CPU-complete. The snapshot retains both recording completion owners without
retaining a recursive chain of frames. Updates, presentation-only work, reused
slots and fingerprint queries do not supply previous camera state.

First use, gaps, history invalidation/cuts, changed projection/extent, invalid camera
matrices and unsubmitted/abandoned predecessors invalidate previous camera state.
Nonfinite, singular or non-affine instance transforms invalidate only that receiver;
geometry/material/token compatibility remains required. Reflected and nonuniform
scale matrices are captured when invertible, but previous-normal raster support and
native acceptance remain pending. Explicit camera cuts/teleports must call
`invalidateHistories()` (and reset upscaler history as required by its API).

Current consumers use optional receiver metadata for rigid reprojection and opaque
upscaler motion as described below. Metadata replay remains opt-in/default off;
capability failures retain the applicable legacy path.

## Rigid metadata generation (P2.2)

`TemporalSurfacePass` now uses the explicitly versioned `surface_v2` shader family
and `draw.temporal_surface.v2` layout. The original `surface` shaders remain
unchanged; a custom shader root must supply the v2 family or initialization reports
unavailable. The GBuffer and its six colors/depth index are unchanged.

The dedicated 288-byte std140/Metal uniform contains uint4 identity at byte 0,
previous model at 16, current unjittered view-projection at 80, previous unjittered
view-projection at 144, current-to-previous normal transform at 208 and float4
internal extent at 272. C++ and Metal assert size/offsets; GLSL specifies offsets.
The same allocation is bound at Vulkan set 2 bindings 0 (vertex) and 1 (fragment),
Metal vertex buffer 9 and fragment buffer 8. Distinct Metal indices are required
because ResourceSet identifies uniform entries by buffer index, across stages.
Frame/object bindings are unchanged. Normal texture/sampler reuse the GBuffer
set 1 bindings 5/11, Metal texture/sampler 3, with the exact material UV/decode path.

Outputs are token R32_UINT, motion RG16F, previousPosition RGBA32F and previousNormal
RGBA16F plus depth. All colors require sampled/color-attachment support,
four simultaneous attachments and the backend-reported aggregate attachment bytes.
A zero aggregate limit means no additional limit in the existing GPU contract.
No new GPU capability API or implicit hardware-support claim is introduced.
Color outputs clear over the full receiver extent and retain independent pooled
owners for the opaque domain and one complete transparent receiver layer. Opaque replay uses private
writable depth. Transparent replay shares the acquired slot's preserved opaque
depth, loads it without clearing, and disables depth writes. It neither allocates
nor seeds a private depth image per transparent item. Ordinary GBuffer transparent
calls keep depth writes disabled. RendererDeferred instead sets
`RasterFrame::captureReceiverDepth` and reuses the existing original G-buffer depth
image as writable scratch, seeded from preserved opaque depth before the receiver
batch and before each composition item. Effects use the batch's actual receiver
depth; lighting uses the individual item's actual receiver depth. No additional
depth image or effect domain is allocated. These copies add bandwidth, not VRAM.
The preserved opaque image is never modified by transparent recording. Temporal
consumers and transparent diagnostics read the batch scratch depth before per-item
replay reuses it. Public metadata replay depth remains the shared opaque visibility
attachment. Shared depth follows the slot's reuse lifetime; color metadata remains independently retained. Retain all consumed
image owners through GPU completion. Signal histories and color metadata are
independent between the opaque and transparent domains, never between individual
transparent objects. Diagnostics use layer 0 for opaque and layer 1 for transparent;
legacy nonzero diagnostic layer values normalize to the shared transparent layer.

`TemporalSurfaceInstance` adds optional trailing previousWorld/hasPreviousWorld;
legacy aggregate callers without them mean a static current model. `record` adds
an optional trailing `TemporalSurfaceCamera` containing the two unjittered
view-projections (identity defaults require an identity camera). RendererDeferred
supplies the immutable P2.1 submitted camera/model snapshots explicitly.
Raster position still uses the current jittered frame binding. Perspective-correct
varyings carry both unjittered clips and previous world position; fragment code
performs the divides and scales previous-current NDC by `(0.5,-0.5)*internalExtent`.
Normals reproduce GBuffer mapping before applying
`transpose(inverse(previousModel)) * transpose(currentModel)` to the current world
normal, equivalent in xyz to `normalMatrix_prev * inverse(normalMatrix_now)`.
This preserves the same-local-shading-normal approximation under affine scale;
nonuniform-scale mapped-normal visual acceptance remains unverified.

Missing chronology, incompatible resources and invalid current/previous matrices
reject locally. Singular/non-affine/nonfinite models and reflected (negative
determinant) models are conservatively unsupported. Invalid clip w, nonfinite
fragment values, degenerate normals and motion exceeding RG16F finite range
emit token zero and position/normal w=0 while retaining receiver depth coverage.
Valid moving receivers keep their canonical nonzero token.

Metadata generation does not itself accumulate history. The accumulator consumes
supported previous-position/normal metadata through its v3 or later path. Injected
legacy consumers cannot interpret moving receiver tokens alone: the renderer drops
metadata when the consumer lacks reprojection and the domain is not
`legacyCompatible`. Static compatible domains may still use token-only metadata.

Static verification passed; compilation, shader compilation, Vulkan/Metal execution,
format support, visual quality and performance remain not_run. Native follow-up:
run the surface fixture and `runRigidSurfaceReferenceProbe`, compare the new images
against `rigidSurfaceReference` for translations/rotations/scales and perspective
triangles, inspect motion sign/jitter independence and normal maps, then exercise
invalid transforms and transparent/opaque retained owners across queued slots.

## P2.3 consumer integration

The P2.2 transitional consumer gate is now replaced by optional v3 reprojection.
The renderer-owned accumulator provisions draw.temporal.v3 and consumes the full
retained previous-position/normal pair returned by temporalSurfaceMetadata for the
exact opaque/transparent domain. Replay still defaults off. Injected legacy
components keep static/capability fallback. Slot-captured camera matrices now feed
recordLinear's reconstruction/effect/denoise and transparent view calculations.
See [TemporalAccumulator](TemporalAccumulator.md#expected-previous-surface-consumption-p23)
for the 352-byte ABI, previous-camera tolerance, local compatibility switches,
explicit camera-cut requirement and validation limits. Opaque motion is also
consumed by the current FSR and MetalFX integrations.

## P2.4 opaque upscaler input

The final UpscalerInput now borrows the retained opaque motion/token pair captured
before transparent passes, with matching strong owners. FsrUpscaler produces a
complete motion field by selecting finite object vectors at nonzero tokens and the
unchanged camera/depth result elsewhere. Replay remains default off and FSR does
not enable it automatically. Transparent/refraction motion remains approximate.
MetalFX uses the same opaque/camera selection contract.
See [Upscaler](Upscaler.md#opaque-object-motion-for-fsr-p24) for ownership, validation,
SDK units/jitter conventions and native acceptance limits.

## Phase 2 diagnostic views (P2.5)

The frame-latched temporalDiagnosticView selector now also exposes Motion, Expected
previous position and Expected previous normal. temporalSurfaceDiagnosticsAvailable
reports their separate optional display kernel; original token/local acceptance
availability remains unchanged. temporalSurfaceMetadata snapshots additionally
retain motion. Motion is visualized around neutral gray (xy/64), position by signed
bounded world axes and previous normal by signed-to-color mapping. Invalid token/
metadata is black. Use Raw HDR for unprocessed diagnostic color. Replay and effect
selection remain explicit, with default replay off and no ordinary readback/wait.
See TemporalAccumulator.md and the Phase 2 P2.5 acceptance matrix. Phase 2 source
work is implemented_unverified; compile/native/visual/performance checks have not run.


## Resolution, reconstruction and final processing

See [Upscaler](Upscaler.md) for the full API and [Settings](Settings.md) for live
method/mode changes. A null configuration upscaler retains a disabled FSR instance
on Vulkan or MetalFX instance on Metal. `ResamplingUpscaler` is independently
initialized as the fallback. The method selects None/FSR/MetalFX; the mode selects
100/60/55/50/35 percent per dimension. These are separate from effect quality.
Initialization resolves the internal extent before creating layer targets.

`recordLinear` composes HDR at internal extent and reconstructs to output RGBA16F.
It passes preserved opaque depth and the captured unjittered/jittered camera pair,
plus optional retained opaque motion/token metadata. SDK failure/unavailability
uses bilinear; temporal diagnostics also bypass SDK reconstruction. A successful
upscaler record with `requirements().jitter` sets the slot's
`temporalUpscaleApplied`. `recordFinal` always executes final tone/color conversion,
then runs an injected `FinalProcessor` only when that flag is false, and finally
records overlays. There is no automatic SMAA insertion or second tone map in an SDK.

`initializationWarnings()` reports recoverable initialization failures and their
fallbacks. `upscaler()->available()`/`status()` report the active implementation;
selector compatibility alone does not prove a requested ratio/camera works.
`setCameraParameters` supplies near/far/FOV metadata for reconstruction; it does
not replace `setCamera`'s view/projection matrices. Keep them consistent.

## Guided reconstruction of low-resolution GI

The private [GI upsampling stage](GIUpsampling.md) runs after temporal/denoise and
before lighting, independently of full-image reconstruction. It reconstructs a
smaller built-in RTGI signal to internal extent using a 4×4 joint bilateral filter
with current normal/depth guides. Opaque and shared transparent receivers each
use their actual captured scratch depth. No extra history is introduced.
Only exact built-in GBuffer/RTGI types are admitted; unsupported initialization
keeps original scaled signal sampling and records a warning.

## Current temporal options

The default accumulator requests optional metadata v2, rigid reprojection v3,
adaptive v4 and moment v5 shader interfaces. Provisioning an interface is distinct
from enabling its policy: receiver replay defaults off and
`TemporalSettings::adaptiveEnabled` defaults false. Availability queries and the
selected diagnostic view report which optional paths can execute. Current views
include tokens, local history, motion, expected previous position/normal, current
weight, reactivity, variance and effective count. See
[TemporalAccumulator](TemporalAccumulator.md) for individual signal/history rules.
