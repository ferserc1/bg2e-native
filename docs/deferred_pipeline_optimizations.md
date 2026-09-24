# Deferred Pipeline — RT Performance Optimization Proposals

This document collects performance optimization proposals for the deferred
renderer (`bg2e::render::RendererDeferred`), focused on the ray-traced global
illumination and reflection shaders, the temporal accumulation / denoising
chain, and the overall RT pipeline architecture. It is a companion to
`doc/deferred_render_model.md`.

> **Status**: the original proposals A1 (hit-shader shadow samples), A3
> (reflection resolution scaling) and A4 (blue-noise sampling) have been
> **implemented** and removed from this document. See
> `doc/deferred_render_model.md` §1, §7–§10 for the implemented behavior.
> Section letters are kept stable so cross-references remain valid.

Impact estimates are **relative to the cost of each pass**, not total frame
time, and must be validated by profiling. Items marked *(configurable)* can be
exposed as quality settings so any quality loss is opt-in.

---

## 0. What is already configurable (verification notes)

Before proposing changes, the existing configurability was re-checked:

- **Per-light shadow ray count** is configurable via
  `base::Light::shadowSamples` for the primary composite, and the GI /
  reflection hit shaders have their own independent caps
  (`RTGISettings::shadowSamples`, `RTReflectionSettings::shadowSamples`,
  default 1) — implemented from the original A1 proposal.
- **GI sample count, bounce count, ray bias, max distance, quality, hit-shadow
  samples and blue-noise toggle** are configurable (`RTGISettings`).
- **Reflection sample count, max roughness, ray bias, max distance, roughness
  spread, resolution quality (implemented from A3), hit-shadow samples and
  blue-noise toggle** are configurable (`RTReflectionSettings`).
- **RTAO** sample count, bounce count, radius, bias, falloff, resolution
  quality and blue-noise toggle are configurable (`RTAmbientOcclusion.hpp`).
- **Blue-noise sampling** is implemented (original A4): a shared
  `bg2e::render::BlueNoise` texture with a per-pass `useBlueNoise` toggle
  (default on) and white-noise fallback.
- **Temporal accumulation** mode (Interactive/Progressive), history weight and
  rejection thresholds are configurable (`TemporalAccumulator.hpp`).

The consequence: several proposals below are not "add a setting" but "make the
existing low-sample settings actually viable" (fixed temporal reprojection) or
reduce the cost of what remains.

---

## A. High impact — GI and reflection shaders

### A2. Explicit texture LOD in hit shaders

> **On hold**: it has not been verified whether material textures are
> generated with mipmaps. This optimization requires mipmapped textures to
> be effective and safe.

`texture(albedoTex[nmatIdx], scaledUV)` in both hit shaders runs with
undefined/zero derivatives, so the driver typically samples LOD 0. Bounce rays
do not need full-resolution albedo. Replace with
`textureLod(..., scaledUV, 3.0)` (configurable mip bias).

Expected impact: **10–30% of hit-shader time** (VRAM traffic and texture
cache pressure); no visible quality loss for 1–2 bounce GI.

Files to examine:
- `shaders/src/glsl/rt_gi.rchit.glsl:98`,
  `shaders/src/glsl/rt_reflections.rchit.glsl:103`
- `shaders/src/glsl/lib/rt_material_data.glsl` (`sampleRTLightEmission` has the
  same issue)

### A5. Payload slimming

`GIPayload` (`rt_gi.rgen.glsl:26-32`) carries 6×vec3 + uint (~100 bytes);
payload size directly affects `traceRayEXT` register pressure and traversal
occupancy.

- Drop `hitPosition` (recompute as `origin + dir * hitT`; pass `hitT`).
- Pack `hitAlbedo` into `uint` (RGB9E5 / R10G10B10A2).
- Target ≤ 48 bytes. `ReflectionPayload` (~24 B) is already small.

Expected impact: **10–25% of trace cost**, more on register-starved GPUs. No
quality loss.

Files to examine:
- `shaders/src/glsl/rt_gi.rgen.glsl`, `shaders/src/glsl/rt_gi.rchit.glsl`
  (payload struct must match in both)
- `shaders/src/glsl/rt_reflections.rgen.glsl`,
  `shaders/src/glsl/rt_reflections.rchit.glsl`

---

## B. Medium impact — shader micro-optimizations

### B1. Russian roulette / early-out in the GI bounce loop *(configurable)*

`rt_gi.rgen.glsl:66-122` runs a fixed bounce count even when `throughput` is
near zero (dark albedos). Add russian-roulette termination after bounce 1
(`if (maxComponent(throughput) < threshold) break`).

Expected impact: **10–20% of GI rays in dark scenes**; minor bias, can be
compensated.

Files: `shaders/src/glsl/rt_gi.rgen.glsl`,
`lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp` (threshold
setting), `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp` (push
constants).

### B2. Adaptive GI sample count *(configurable)*

Mirror what reflections already do (`rt_reflections.rgen.glsl:88-89`): scale GI
samples per pixel — fewer samples where the surface normal faces the sky
(high miss probability → irradiance map dominates) or where accumulated
history is stable (requires a history-length texture, see D1).

Expected impact: **20–40% of GI rays.**

Files: `shaders/src/glsl/rt_gi.rgen.glsl`,
`lib/src/bg2e/render/deferred/TemporalAccumulator.cpp` (optional history-length
output).

### B3. Convert RTGI to a compute shader with `rayQueryEXT`

RTAO already proves the pattern (`rt_ao.comp.glsl`). A compute GI:
- eliminates raygen/SBT dispatch overhead and payload spills,
- keeps bounce state in registers instead of re-invoking hit shaders via
  payload,
- enables subgroup ops and shared-memory G-buffer tiles.

Hit shading would need manual material fetch — the code already exists in
`rt_gi.rchit.glsl` and can be moved to a shared include.

Expected impact: **15–30% of GI dispatch overhead** (hardware-dependent;
biggest where `traceRayEXT` pipelines have high fixed cost). No quality
change. Moderate refactor.

Files to examine:
- `shaders/src/glsl/rt_ao.comp.glsl` (pattern reference)
- `shaders/src/glsl/rt_gi.{rgen,rchit,rmiss}.glsl`
- `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp` (replace RT pipeline
  with compute pipeline, drop SBT)
- `lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp` (compute + rayQuery
  host-side reference)

### B4. Bilateral denoiser: shared-memory tile or separable

`denoise_bilateral_hdr.comp.glsl:66-88` does 5×5 = 25 taps × 3 texture reads
(input, normal, depth) = 75 fetches/pixel at full res.

- Option 1 (easy): load a (8+2r)² tile into shared memory, read normal/depth
  once per texel → ~3× fewer texture fetches. **Impact: 30–50% of the
  denoise pass.**
- Option 2: separable H+V passes (10 taps); slight quality change on diagonal
  edges, configurable.

Files: `shaders/src/glsl/denoise_bilateral.comp.glsl`,
`shaders/src/glsl/denoise_bilateral_hdr.comp.glsl`,
`lib/src/bg2e/render/deferred/DenoiseFilter.cpp`.

### B5. Shadow cone-ray coherence in `queryShadow` *(configurable)*

Up to 32 soft-shadow rays per light per pixel in the composite
(`deferred_composite_rt.frag.glsl:211-223`).

- Early-exit the sample loop when the first N samples agree (fully lit or
  fully occluded) — most pixels are not in penumbra. **Impact: 30–60% of
  composite shadow cost.**
- Scale effective `shadowSamples` by light distance / screen contribution.

Files: `shaders/src/glsl/lib/ray_tracing.glsl`,
`shaders/src/glsl/deferred_composite_rt.frag.glsl`.

### B6. Roughness-adaptive reflection ray distance *(configurable)*

Leftover from the original A3 (the resolution-scaling part is implemented):
rough reflections are low-frequency, so their rays rarely need the full
`maxDistance` (50.0). Scale the effective `tMax` per pixel by roughness
(e.g. `mix(10.0, maxDistance, 1 - roughness)`).

Expected impact: **10–25% of reflection traversal cost** in scenes with
distant geometry; minor quality loss on rough surfaces, configurable.

Files: `shaders/src/glsl/rt_reflections.rgen.glsl`,
`lib/include/bg2e/render/deferred/RTReflections.hpp`,
`lib/src/bg2e/render/deferred/RTReflections.cpp`.

---

## C. Architecture / pipeline

### C1. Overlap AO/GI with reflections

In `DeferredLayer::render()` the chain is strictly serial: GI → GI accumulate
→ GI denoise → reflections → reflection accumulate → composite. But **GI/AO
and reflections are independent of each other** (both consume only the
G-buffer and TLAS).

- Issue the reflection trace right after the GI trace (before GI
  accumulate/denoise) so both trace dispatches can overlap on the GPU; only
  the accumulations must precede the composite. Requires only reordering +
  correct barriers.
- Longer term: an async compute queue (note the shared-history constraint
  documented in `doc/deferred_render_model.md` §11).

Expected impact: **hides up to ~½ of combined RT trace time** on GPUs with
spare compute units; near-zero cost in the reorder variant.

Files: `lib/src/bg2e/render/deferred/DeferredLayer.cpp` (`render()`,
`:325-424`).

### C2. Run temporal accumulation + denoise at the *scaled* RT resolution

AO/GI render at quality-scaled resolution, but accumulation and denoise run at
**full layer extent**, bilinearly upsampling the low-res input — full-res
price for ¼ of the information.

- Accumulate and denoise at the AO/GI native (scaled) resolution; upsample
  once in the composite (it already samples the result as a texture).
- HDR history images also shrink 4–9× → less bandwidth and memory.

Expected impact: **accumulation + denoise become 4–9× cheaper** relative to
their current cost (at Ultra/1.0 there is no change, so pair with making High
the default). No quality loss — the input is already low-res.

Files to examine:
- `lib/src/bg2e/render/deferred/TemporalAccumulator.cpp` (extent handling,
  `:205-362`)
- `lib/src/bg2e/render/deferred/DenoiseFilter.cpp`
- `lib/src/bg2e/render/deferred/DeferredLayer.cpp` (`:335-395`, wiring of
  image sizes)
- `shaders/src/glsl/temporal_accumulation.comp.glsl`,
  `denoise_bilateral*.comp.glsl` (UV scaling between input/output sizes)

### C3. Half-resolution + checkerboard tracing *(configurable quality mode)*

Trace GI/reflections at half res with a per-frame checkerboard offset;
temporal accumulation fills the missing texels. Effectively 2 spp over 2
frames for the price of 0.5.

Expected impact: **~2× trace cost reduction**; slight latency in GI response,
configurable.

Files: the scaled-resolution plumbing from the implemented A3 plus a
frame-parity offset in the push constants of `RTGlobalIllumination.cpp` /
`RTReflections.cpp`.

### C4. Unified RT pipeline (long-term)

GI, reflections and (optionally) composite shadows could share a single RT
pipeline/SBT with different ray types. Saves pipeline binds and descriptor
re-binds between passes, and allows shading a GI ray and a reflection ray per
pixel in one launch.

Expected impact: **5–15% fixed overhead reduction**; high complexity — only
after A/B items land.

Files: `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp`,
`RTReflections.cpp`,
`lib/include/bg2e/render/vulkan/factory/RayTracingPipeline.hpp`, all
`rt_*.rgen/rchit/rmiss` shaders.

---

## D. Cross-cutting

### D1. Fix temporal history invalidation

`TemporalAccumulator.cpp:220-232` drops history on **any** camera matrix
change (ε = 0.001), yet the shader already does full reprojection with
depth/normal rejection (`temporal_accumulation.comp.glsl:83-112`). While the
camera moves, the renderer runs on raw 1–4 spp noise, and Progressive mode
never converges unless the camera is perfectly static.

- Remove the blanket host-side invalidation; rely on shader reprojection +
  rejection (optionally keep a threshold on *large* motion).
- Optionally add variance clipping (color-space AABB clamp of history) to kill
  ghosting instead of rejecting.

Expected impact: not a direct speedup, but raises the effective sample count
5–10× during motion → **enables reducing the per-frame sample counts** now
that blue noise (implemented A4) is in place. Risk: ghosting on
disocclusions, mitigated by the existing depth/normal tests; configurable
via `historyWeight`.

Files: `lib/src/bg2e/render/deferred/TemporalAccumulator.cpp`,
`lib/include/bg2e/render/deferred/TemporalAccumulator.hpp`,
`shaders/src/glsl/temporal_accumulation.comp.glsl`.

### D2. G-buffer normal packing

The normal attachment is `R16G16B16A16_SFLOAT` (8 B/px), read by AO, GI,
reflections, accumulation ×2 and denoise ×2. Octahedral encoding into
`R16G16` (4 B/px) halves that bandwidth in 6+ passes.

Expected impact: **a few % of total frame**; moderate refactor touching many
shaders.

Files: `lib/src/bg2e/render/gbuffer/GBufferManager.cpp`,
`shaders/src/glsl/deferred_gbuffer.frag.glsl`,
`shaders/src/glsl/lib/deferred_utils.glsl`, every consumer shader (AO, GI,
reflections, accumulation, denoise, composite).

### D3. FSR reset flag (correctness, free)

`FSRPostProcessor` never sets `dispatch.reset` — wire it to camera-cut
detection. No perf cost, removes ghosting on cuts.

Files: `lib/src/bg2e/render/deferred/FSRPostProcessor.cpp` (`process()`,
`:314-433`).

---

## E. Bugs found during the analysis

### E1. Non-shadow-casting lights contribute nothing to reflections

`rt_reflections.rchit.glsl:114-119`: `shadowFactor` initializes to **0.0**, so
lights with `castShadows == 0` are completely absent from reflections (the GI
hit shader correctly initializes to 1.0 at `rt_gi.rchit.glsl:114`). Reflected
surfaces are missing all non-shadow-casting lights. Correctness bug; fixing it
slightly *increases* cost but the current output is wrong.

Files: `shaders/src/glsl/rt_reflections.rchit.glsl`.

### E2. Soft-shadow noise is static per surface point

`queryShadow` hashes `worldPos.xz` (`lib/ray_tracing.glsl:102`) — the seed is
constant per surface point across frames, so penumbra noise never converges
temporally. Mixing `frameIndex` into the seed (requires plumbing it into the
composite / hit shaders) lets temporal accumulation clean the penumbra, after
which 32 samples can drop to 4–8. **Configurable; ~2–4× cheaper soft
shadows.**

Files: `shaders/src/glsl/lib/ray_tracing.glsl`,
`shaders/src/glsl/deferred_composite_rt.frag.glsl`,
`shaders/src/glsl/rt_gi.rchit.glsl`,
`shaders/src/glsl/rt_reflections.rchit.glsl`,
`lib/src/bg2e/render/deferred/DeferredLayer.cpp` (`CompositePushConstants`,
`DeferredLayer.hpp:258-270`), `RTGlobalIllumination.cpp` / `RTReflections.cpp`
push constants.

---

## Suggested priority

| Order | Item | Effort | Impact |
|---|---|---|---|
| 1 | E1 / E2 bug fixes | Low | Correctness + high |
| 2 | C2 accumulate/denoise at scaled res | Medium | High |
| 3 | D1 history invalidation fix | Low | High (enabler) |
| 4 | A2 texture LOD, A5 payload, B1/B2/B6 | Low | Medium |
| 5 | C1 pass overlap | Medium | Medium-high |
| 6 | B3 GI-as-compute, C4 unified pipeline | High | Medium |
