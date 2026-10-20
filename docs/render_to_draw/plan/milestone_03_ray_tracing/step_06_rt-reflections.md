# M03-06 — Public traced reflections and resolution semantics

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-01–M03-03/M03-05 shared hit data.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/RTReflections.cpp`
- `lib/include/bg2e/render/deferred/RTReflections.hpp`
- `lib/include/bg2e/draw/pipeline/RTReflections.hpp`
- `shaders/src/draw/rt_reflections/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/RTReflections.cpp](../../../../lib/src/bg2e/render/deferred/RTReflections.cpp), [lib/include/bg2e/render/deferred/RTReflections.hpp](../../../../lib/include/bg2e/render/deferred/RTReflections.hpp).

## Implementation work

1. Implement explicit backend reflection shaders producing HDR radiance and mask/confidence, with cutout-aware geometry/material hit lookup.
2. Preserve roughness threshold/spread, sample count, bias/distance, quality, shadow-sample/noise and reflection-light participation settings.
3. Keep reflection-quality extent relative to display output, independently of internal G-buffer/FSR scale. Define IBL fallback when traced data is unavailable.
4. Expose compatible shader/resource/subclass configuration and normal accessors; no native Vulkan descriptors in public reflection objects.

## Verification and exit conditions

- Reflective/rough surfaces, misses and alpha-cutout hits compose correctly; output-size-only and internal-scale changes resize the appropriate resources.
- Fallback retains environment specular behavior, and user component injection follows the public pass contract.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03-07 owns reflection histories separately from GI/AO. M05 freezes scene while collecting requested rendered-frame samples.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Later global default-policy amendment

The project lead directed that renderer-created reflections are enabled by
default, independently of the indirect-lighting mode. `RTReflectionSettings` now
defaults `enabled` to true; explicit pass/settings and hardware availability remain
authoritative. See the [current review amendment](review.md#later-default-policy-amendment).

## M01-08 IBL fallback ABI

Use the same validated EnvironmentResources and public services from [environment-tools.md](../milestone_01_deferred_renderer/environment-tools.md). Prefilter mips encode perceptual roughness linearly; BRDF RG is A/B at (NdotV, roughness) with F0*A+B composition. Sky/prefilter are linear radiance; irradiance includes PI. Retain snapshots across submitted work and never overwrite authored prefilter mips or synchronously rebuild while recording RT work.

## M03-01 checkpoint handoff — accepted by lead continuation

Use the same accepted [M03-01](rt-design.md) and M03-04 snapshot/candidate/material contracts as GI, with native Vulkan stages and Metal nearest-hit compute. Define tangent/normal-map orientation including mirrored transforms, explicit LOD sampling and environment miss behavior. Reflection quality remains output-relative, independent of internal G-buffer/FSR scale. HDR radiance and confidence have separate semantics; preserve IBL for unavailable/rejected samples. Do not introduce another material table or transparent transport policy.

## M03-02 actual primitive contract

Read [rt-gpu-data.md](rt-gpu-data.md) with the later [M07 capacity amendment](../milestone_07_bg2e_composer/tracing-capacity-16384.md). Choose the exact device-clamped capacity up to 16384 after reserves, not small compiled effect profiles. Vulkan specializes ID0; Metal uses separate argument record buffers8/9. Keep complete arrays, retained owners, retired/drained updates and warning/neutral-texture overflow behavior. Primitive probe profiles are not the engine ABI. No global material ownership, buffer-address or array graphics-stage infrastructure is implied.

## M03-03 actual noise input

Reuse [utils::BlueNoise](blue-noise.md): CPU layer-major rank data and an explicitly shared RGBA8 color 2D atlas, with uint4 metadata and paired bg2eBlueNoise2/bg2eHashNoise2 helpers under draw/lib. Logical layers are stacked in Y; physical arrayLayers is one. Do not copy production sampler2DArray bindings or assume general image-array support. Effects own rendered-view/sample keys and disabled/missing-noise variants without texture access; the utility does not advance presentation/slot state or reserve tracing-table capacity implicitly. Retain image/sampler owners and metadata snapshots through completion.

## M03-04 actual implementation handoff

Use [rtao.md](rtao.md): public RTAmbientOcclusion, SurfaceInputs camera/explicit profiles and TracingSceneResources/TracingSnapshot now exist. Reuse dense IDs, retained material tables, scalar R16 output and explicit renderedSample keys. AO has independent quality dimensions via ExtentDomain::Scaled. No history/filtering or visible controls are implemented here. Legacy SoftShadowPass retains its separate eight-cutout-pair limit; reconcile that consumer during integration/review. Native pipeline GLSL users may include tracing.glsl without DRAW_TRACE_QUERIES; compute-query users define it first.

## M03-05 actual GI handoff

[rtgi.md](rtgi.md) records public RTGlobalIllumination, native Vulkan raygen/miss/closest-hit/any-hit with depth one, Metal compute queries, shared material tables and explicit receivers. RGBA16 output is incoming diffuse radiance/pi before receiver albedo with alpha validity; invalid zero restores diffuse IBL. RendererDeferred selects IBL/AO/GI independently of retained settings, preserves specular IBL and supports scaled GI inputs. History/filter/UI and baked presence policy remain pending. Reuse this exact normalization and matching-capacity snapshot ownership.

## M03-06 delivered contract

[rt-reflections.md](rt-reflections.md) records RTReflections, native stages, display-relative quality, source normal/tangent convention, unpremultiplied HDR hit radiance and fractional confidence, lifecycle and validation limits. Delivery awaits project lead compilation/runtime validation.
