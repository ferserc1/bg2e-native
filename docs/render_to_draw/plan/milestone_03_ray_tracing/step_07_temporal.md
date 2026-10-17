# M03-07 — Rendered-view histories and temporal accumulation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-04–M03-06 and M01 runtime delegate sequence.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/TemporalAccumulator.cpp`
- `lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp`
- `lib/include/bg2e/draw/pipeline/MotionVectorGenerator.hpp`
- `shaders/src/draw/temporal_accumulation/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/TemporalAccumulator.cpp](../../../../lib/src/bg2e/render/deferred/TemporalAccumulator.cpp).

## Implementation work

1. Implement public scalar/HDR accumulation with depth/normal rejection and separate histories per effect/layer/view. Track rendered-scene sequence independently of presentation frame slots.
2. Reset on camera cuts, renderer/upscaler replacement, extent/quality/mode changes and relevant mesh/material/light/environment edits; honor pause/retained-scene behavior.
3. Use completion-safe mutable shader/output/history ownership, not rings of mesh/texture assets. Preserve actual sceneFrameNumber, previous/current unjittered matrices and SDK jitter conventions. RendererDeferred targets/camera are private: define a public borrowed signal/surface profile when sharing these inputs, with reset generations. FSR SDK context state is not the engine temporal accumulator.
4. Expose accumulation controls and replacement configuration. Define bounded convergence/frame-count behavior for later headless jobs; record whether camera-only motion suffices or moving-object vectors need expansion.

## Verification and exit conditions

- Static results converge; resize/edit/camera discontinuity resets correctly; UI-only presentation and paused frames do not advance sample chronology.
- AO, GI and reflection histories cannot overwrite each other and FSR switching resets its own SDK state consistently.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04 UV accumulation remains a distinct bake signal, M05 selects actual rendered accumulation frames, and apps must notify the real reset paths.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-05 lifecycle handoff

Use M01-05's `FrameContext::sceneFrameNumber` for submitted scene history progression; frameNumber remains a presentation sequence and advances during retained-image/UI-only presentations. Renderer/scene/extent changes still require explicit history resets even though sceneFrameNumber remains contiguous across renderer replacement. Follow [runtime-renderer.md](../milestone_01_deferred_renderer/runtime-renderer.md) lifecycle boundaries and sceneDeltaSeconds timing.

## M01-13 FSR coexistence

Read [upscaling.md](../milestone_01_deferred_renderer/upscaling.md). FSR owns SDK temporal state and camera-only motion; it is not the engine history abstraction. Extend object motion/reactive/reset information through the open Upscaler contract without closing external injection. Preserve sceneFrameNumber jitter, current jittered versus previous unjittered clip, previous-minus-current framebuffer pixel motion and negative-height Vulkan viewport Y. Transparent last-layer depth is not the preserved opaque depth. Coordinate camera cuts/discontinuous edits with RendererDeferred::resetUpscaler at a safe boundary.

## M02-04 material edit invalidation

[MaterialModifier](../milestone_02_manipulation/material-modifier.md) patches CPU/live
attributes independently at stopped boundaries. The application queues generation-
checked edits and requests redraw/reset even after partial failure; resetUpscaler
is the current hook. Extend this edit boundary to new effect histories rather
than expecting the utility to retain renderer/scene ownership.

M02-05's DrawMaterialEditor also validates renderer generation, binding revision
and weak selected-resource identity, and invokes its `onChanged(Renderer&)`
callback after queued changes (including partial failure). M02-07 demonstrates
the application adapter. M03 effect histories need an explicit app notification
that resets each affected history at the same safe boundary; do not couple the
generic MaterialModifier utility to renderer history ownership. Geometry/topology
replacement additionally invalidates selection identities and requires fresh
extraction, not only a material-history reset. See [M02 review](../milestone_02_manipulation/review.md).

## M03-01 checkpoint handoff — accepted by lead continuation

Implement the accepted [M03-01 mutation/reset contract](rt-design.md): a public effect/renderer invalidation entry point, tracing scene revisions, per-rendered-view/layer sequence, independent AO/GI/reflection validity and previous-object correspondence. Material/light/environment/texture edits at the drained pre-acquire boundary update snapshots and reset affected histories; geometry/scene replacement also rebuilds acceleration data. Distinguish camera cuts, resize/quality/pass changes, pause and pick/overlay-only frames. Coordinate FSR resets without treating resetUpscaler or acquired-slot indexing as the effect-history API.

## M03-03 actual noise input

Reuse [utils::BlueNoise](blue-noise.md): CPU layer-major rank data and an explicitly shared RGBA8 color 2D atlas, with uint4 metadata and paired bg2eBlueNoise2/bg2eHashNoise2 helpers under draw/lib. Logical layers are stacked in Y; physical arrayLayers is one. Do not copy production sampler2DArray bindings or assume general image-array support. Effects own rendered-view/sample keys and disabled/missing-noise variants without texture access; the utility does not advance presentation/slot state or reserve tracing-table capacity implicitly. Retain image/sampler owners and metadata snapshots through completion.

## M03-04 actual implementation handoff

Use [rtao.md](rtao.md): public RTAmbientOcclusion, SurfaceInputs camera/explicit profiles and TracingSceneResources/TracingSnapshot now exist. Reuse dense IDs, retained material tables, scalar R16 output and explicit renderedSample keys. AO has independent quality dimensions via ExtentDomain::Scaled. No history/filtering or visible controls are implemented here. Legacy SoftShadowPass retains its separate eight-cutout-pair limit; reconcile that consumer during integration/review. Native pipeline GLSL users may include tracing.glsl without DRAW_TRACE_QUERIES; compute-query users define it first.

## M03-05 actual GI handoff

[rtgi.md](rtgi.md) records public RTGlobalIllumination, native Vulkan raygen/miss/closest-hit/any-hit with depth one, Metal compute queries, shared material tables and explicit receivers. RGBA16 output is incoming diffuse radiance/pi before receiver albedo with alpha validity; invalid zero restores diffuse IBL. RendererDeferred selects IBL/AO/GI independently of retained settings, preserves specular IBL and supports scaled GI inputs. History/filter/UI and baked presence policy remain pending. Reuse this exact normalization and matching-capacity snapshot ownership.

## M03-06 actual reflection handoff

Use [rt-reflections.md](rt-reflections.md): public RTReflections reuses retained scene tables and camera/explicit surfaces, with receiver roughness and camera position. Quality scales display output independently of G-buffer/internal/FSR extent. RGBA16 RGB is mean geometry-hit radiance before receiver BRDF, unpremultiplied; alpha is finite accepted hits/actualSamples. Preserve fractional confidence and rejected masks separately from GI binary validity and AO scalar visibility. The lighting pass blends prefiltered IBL with traced radiance using confidence and applies split-sum BRDF once. FullscreenPass supports an optional independent Metal sampler map; reflection texture16 shares nearest sampler0. Histories/filter/UI remain pending.

## M03-07 delivered contract

[temporal.md](temporal.md) records public inputs, independent keys, immutable history ownership, actual scene sequencing, reprojection/normal-position rejection, fractional confidence and bounded counts. Moving-object vectors are not implemented: transform/geometry changes conservatively reset effects and FSR. MotionVectorGenerator is the public CPU camera reference; existing FSR GPU motion generation remains intact. Lead compilation/runtime validation is pending.
