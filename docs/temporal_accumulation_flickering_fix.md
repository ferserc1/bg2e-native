# Temporal Accumulator Flickering Fix — Technical Change Report

**Scope:** `bg2e::render::deferred::TemporalAccumulator` and its call sites.
**Goal:** eliminate the image flickering caused by maintaining one independent
temporal accumulation chain per frame-in-flight, by moving to a **single
shared history chain** per `TemporalAccumulator` instance.
**Approach:** Option A (shared global temporal state), as selected in the
analysis phase.

This document describes *what* must change in each file and code block, and
*why*. It is intentionally not a step-by-step implementation plan.

---

## 1. Background and root cause

### 1.1 Current design

`TemporalAccumulator`
(`lib/include/bg2e/render/deferred/TemporalAccumulator.hpp`,
`lib/src/bg2e/render/deferred/TemporalAccumulator.cpp`) replicates **all** of
its temporal state per frame-in-flight (`createHistoryImages()`,
`TemporalAccumulator.cpp:80-165`):

| State | Type | Current cardinality |
|---|---|---|
| `_historyImagesA` / `_historyImagesB` | `std::vector<std::shared_ptr<vulkan::Image>>` | `numImages()` (ping-pong per frame slot) |
| `_prevDepthImages` | vector of images | `numImages()` |
| `_prevNormalImages` | vector of images | `numImages()` |
| `_writeIndex` | `std::vector<uint32_t>` | `numImages()` |
| `_hasHistory` | `std::vector<bool>` | `numImages()` |
| `_accumulatedFrameCount` | `std::vector<uint32_t>` | `numImages()` |
| `_previousViewProjection` | `std::vector<glm::mat4>` | `numImages()` |

At render time the slot is selected with
`frameIndex = _engine->currentFrameResourcesIndex()`
(`TemporalAccumulator.cpp:218`).

### 1.2 Why it flickers

With 2 frames in flight, frame N accumulates against the history written by
frame **N-2** (the previous user of the same slot), not frame N-1. The
stochastic RT passes seed their white noise with the monotonically increasing
`currentFrame` (`rt_ao.comp.glsl`, `rt_gi.rgen.glsl`,
`rt_reflections.rgen.glsl`), so each slot converges with a *different* noise
subsequence (even frames vs odd frames). The two chains therefore converge to
slightly different stable values and alternate on screen → visible flicker /
pulsing, most noticeable in the accumulated AO, reflections and GI.

Secondary defects caused by the same design:

- Reprojection rejection compares against depth/normal from **2 frames ago**
  instead of the immediately previous frame — worse disocclusion handling.
- `Progressive` accumulation mode (`weight = 1/(accumulatedFrameCount+1)`)
  converges at half speed because the counter advances once every 2 frames.
- Double memory footprint for history + prev depth + prev normal images.

### 1.3 Affected instances

Six `TemporalAccumulator` instances exist, all fixed by the same class-level
change:

- Opaque `DeferredLayer`: `_temporalAccumulator` (AO),
  `_temporalReflectionAccumulator`, `_temporalGIAccumulator`
  (`DeferredLayer.cpp:107-108, 125-128, 139-142`).
- Transparent `DeferredLayer`: the same three instances.

---

## 2. Design of the fix (Option A)

Temporal history is conceptually **global renderer state**, not a per
swapchain-image resource. The fix removes the frame-in-flight dimension from
all temporal state so there is exactly one ping-pong chain (A/B), one
previous-depth image, one previous-normal image and one set of scalars per
`TemporalAccumulator` instance.

### 2.1 Why this is safe on the GPU timeline

Frames in flight overlap on the **CPU** (the CPU records frame N+1 while the
GPU still executes frame N), but all work is submitted to the **same graphics
queue and executes in submission order**: the GPU does not begin frame N+1's
command buffer until frame N's has finished. Therefore a single shared
history chain cannot produce a read/write race; the only requirement is that
the image layout transitions inside each frame act as the necessary pipeline
barriers between the write of frame N and the read of frame N+1.

This is already the model used implicitly by the current code within a single
frame (write → `SHADER_READ_ONLY_OPTIMAL` → sampled by the composite); the
fix simply extends that invariant across frame boundaries by keeping the
layout persistent.

> **Constraint to document in code:** if the engine ever moves the
> accumulation dispatch to a separate async compute queue, the shared history
> chain will require explicit cross-queue semaphores. This must be noted in
> the class documentation comment.

### 2.2 Layout/barrier discipline

After the change, both history images and both prev-G-buffer images must
always be in `SHADER_READ_ONLY_OPTIMAL` between frames. Within a frame:

1. **History write image**: transition `SHADER_READ_ONLY_OPTIMAL → GENERAL`
   (storage write) before the dispatch; back to `SHADER_READ_ONLY_OPTIMAL`
   after it. This replaces the current `UNDEFINED → GENERAL` transition
   (`TemporalAccumulator.cpp:237-238`), which was legal because the image was
   slot-private; with a shared chain the old layout is known and persistent.
2. **History read image**: sampled only; no transition needed (it is already
   `SHADER_READ_ONLY_OPTIMAL` — it was left in that layout when it was the
   write image of the previous frame, or at creation time).
3. **Prev depth/normal images**: the existing copy block
   (`TemporalAccumulator.cpp:285-350`) already transitions
   `SHADER_READ_ONLY_OPTIMAL → TRANSFER_DST_OPTIMAL →
   SHADER_READ_ONLY_OPTIMAL`; this stays unchanged, only the per-slot indexing
   disappears.

Both transitions in step 1 generate the memory dependency
(`COMPUTE_SHADER` write → `COMPUTE_SHADER`/`FRAGMENT` read) required for the
next frame's dispatch and for the same frame's composite/denoise consumers.

### 2.3 Deterministic initial state

`createHistoryImages()` currently transitions only history image B to
`SHADER_READ_ONLY_OPTIMAL` (`TemporalAccumulator.cpp:125-130`). With the
shared chain, **both** A and B (and both prev images, already done) must be
transitioned to `SHADER_READ_ONLY_OPTIMAL` at creation, so that the first
frame can treat either image as a valid read source with a known layout. This
also covers the post-`resize()` path, which recreates the images.

---

## 3. Required changes per file

### 3.1 `lib/include/bg2e/render/deferred/TemporalAccumulator.hpp`

**Member declarations (lines 80-90)** — replace every per-frame vector with a
scalar member:

- `_historyImagesA` / `_historyImagesB` → `_historyImageA`, `_historyImageB`
  (`std::shared_ptr<vulkan::Image>`).
- `_prevDepthImages` → `_prevDepthImage`.
- `_prevNormalImages` → `_prevNormalImage`.
- `_writeIndex` → `uint32_t _writeIndex = 0`.
- `_hasHistory` → `bool _hasHistory = false`.
- `_accumulatedFrameCount` → `uint32_t _accumulatedFrameCount = 0`.
- `_previousViewProjection` → `glm::mat4 _previousViewProjection{1.0f}`.

**Private helpers (lines 124-125)** — `historyReadImage(uint32_t)` and
`historyWriteImage(uint32_t)` drop the `frameIndex` parameter.

**`outputImage(uint32_t frameIndex)` (line 53)** — two options:

- *Minimal-diff option (preferred for this change):* keep the signature and
  ignore the parameter, so the existing call sites in `DeferredLayer.cpp`
  (see §3.3) compile untouched. Add a comment noting the parameter is
  deprecated/unused.
- *Clean option:* change the signature to `outputImage()` and update all call
  sites (listed in §3.3). This is a small, mechanical follow-up.

The same consideration applies to the private helpers.

**Documentation** — add a class comment stating that the accumulator keeps a
single global history chain, that this relies on in-order execution on a
single queue, and the async-compute caveat from §2.1.

### 3.2 `lib/src/bg2e/render/deferred/TemporalAccumulator.cpp`

**`createHistoryImages()` (lines 80-165)**:

- Remove the `resize(numImages())` calls and the per-index loop over
  `_engine->numImages()`; create exactly one image per role. The
  `immediateSubmit` block is still required for the initial transitions, but
  its body no longer loops.
- Transition **both** `_historyImageA` and `_historyImageB` to
  `SHADER_READ_ONLY_OPTIMAL` (currently only B is transitioned).
- Image names no longer need the `" " + std::to_string(i)` suffix.
- Formats, extents, usage flags and aspects remain unchanged.

**`render()` (lines 205-362)**:

- Remove `frameIndex = _engine->currentFrameResourcesIndex()` as the selector
  of temporal state (line 218). `frameResources` is still used to allocate
  the per-frame descriptor set (lines 240-256) — that is correct and stays:
  descriptor sets must remain per-frame because they are recorded into the
  frame's command buffer while the previous frame may still be in flight.
  Only the *images referenced* by those descriptor sets become shared.
- All state accesses drop the `[frameIndex]` indexing:
  - Camera-motion invalidation (lines 220-232): compare
    `_previousViewProjection` against the current frame's view-projection.
    Note this now compares against the **immediately previous frame**, which
    also improves reprojection quality (the previous code compared against
    the state of 2 frames ago).
  - `historyReadImage()` / `historyWriteImage()` calls (lines 234-235)
    become parameterless.
  - **History write transition (lines 237-238):** change
    `VK_IMAGE_LAYOUT_UNDEFINED` to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
    as the old layout. This is the single most important barrier change: it
    makes the layout persistent across frames and provides the acquire-side
    dependency on the previous frame's read of the same image.
  - Descriptor set updates (lines 242-255): `_prevDepthImages[frameIndex]` →
    `_prevDepthImage`, `_prevNormalImages[frameIndex]` → `_prevNormalImage`.
  - Push constants (lines 263-274): `_accumulatedFrameCount[frameIndex]` →
    `_accumulatedFrameCount`, `_hasHistory[frameIndex]` → `_hasHistory`.
  - Depth/normal copy blocks (lines 285-350): replace the indexed images with
    the scalar members; transitions and `vkCmdCopyImage` regions are
    unchanged.
  - End-of-frame bookkeeping (lines 352-361): `_writeIndex = 1 - _writeIndex`;
    `_hasHistory = true`; `_accumulatedFrameCount++`;
    `_previousViewProjection = frameViewProj`.

**`resize()` (lines 364-368)** — unchanged structurally (delegates to
`createHistoryImages()`); verify that after recreation the first `render()`
sees the deterministic initial layouts from §2.3 and `_hasHistory == false`
so the first frame blends with `weight = 1` (current behavior already handles
this via the `hasHistory` push constant).

**`cleanupImages()` (lines 396-422)** — scalar version: clean up and reset
the four images and reset the scalar state (`_writeIndex = 0`,
`_hasHistory = false`, `_accumulatedFrameCount = 0`,
`_previousViewProjection = identity`).

**`invalidateHistory()` (lines 434-441)** — scalar version: reset
`_hasHistory` and `_accumulatedFrameCount`.

**`outputImage()` / `historyReadImage()` / `historyWriteImage()` (lines
424-427, 483-500)** — parameterless (or parameter-ignoring) versions
selecting between `_historyImageA`/`_historyImageB` via the scalar
`_writeIndex`; logic otherwise identical.

### 3.3 `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

No behavioral change is required if the minimal-diff signature is kept. The
following call sites pass `frameResourcesIndex` /
`_engine->currentFrameResourcesIndex()` into `outputImage()` and will simply
have the argument ignored (or be updated to the parameterless signature if
the clean option is chosen):

- `resolveDebugSource()` — lines 62-63 (`TemporalAccumulatedAO`), 68-69
  (`TemporalAccumulatedReflections`), 72-73 (`RTReflectionMask`).
- AO chain — lines 347-351.
- GI chain — lines 383-388.
- Reflections chain — lines 417-422.

Nothing else in `DeferredLayer` changes: the accumulation calls
(`_temporalAccumulator->render(...)` etc.), the denoise inputs wired from
`outputImage()`, and the composite binding of the denoised result are all
unaffected. `DenoiseFilter` keeps its per-frame **output** images
(`DenoiseFilter.cpp:60-63`) — that is correct, because its output is written
by a storage-image dispatch and consumed by the composite within the same
frame only; it carries no cross-frame state.

### 3.4 Other files — impact review

| File | Impact |
|---|---|
| `lib/src/bg2e/render/RendererDeferred.cpp` | None. It only forwards parameter setters (`setTemporalMode`, `setTemporalHistoryWeight`, thresholds, lines 707-749) to the layers. |
| `lib/src/bg2e/ui/RenderSettingsWindow.cpp` | None. UI setters only. |
| `lib/src/bg2e/render/RenderSettingsPreferences.cpp` | None. Persistence of the mode enum only. |
| `shaders/src/glsl/temporal_accumulation.comp.glsl` | **No shader change required.** The push-constant block and bindings are identical; only the *values* (`hasHistory`, `accumulatedFrameCount`, `previousViewProjection`) now come from a single global chain. |
| `lib/src/bg2e/render/deferred/FSRPostProcessor.cpp` / `SMAAPostProcessor.cpp` | None. FSR owns its internal history; SMAA has no temporal state. |
| `GBufferManager`, `RTAmbientOcclusion`, `RTGlobalIllumination`, `RTReflections` | None. Their per-frame *output* images remain per-frame (they are regenerated every frame and hold no cross-frame state). |

---

## 4. Synchronization and hazard checklist

The following invariants must hold after the change; they should be verified
during code review and validation-layer runs:

1. **Persistent layouts:** `_historyImageA/B`, `_prevDepthImage`,
   `_prevNormalImage` are always in `SHADER_READ_ONLY_OPTIMAL` at frame
   boundaries. The only excursions are `GENERAL` (history write, during the
   accumulation dispatch) and `TRANSFER_DST_OPTIMAL` (prev depth/normal,
   during the end-of-frame copies), each bracketed by explicit transitions in
   the same command buffer.
2. **Cross-frame WAR hazard (read in frame N → write in frame N+1):**
   covered by the `SHADER_READ_ONLY_OPTIMAL → GENERAL` transition on the
   write image at the start of frame N+1's dispatch.
3. **Cross-frame RAW hazard (write in frame N → read in frame N+1):** covered
   by the `GENERAL → SHADER_READ_ONLY_OPTIMAL` transition at the end of frame
   N's dispatch plus queue submission order.
4. **Descriptor lifetime:** descriptor sets referencing the shared images are
   still allocated from the per-frame `frameResources` allocator
   (`TemporalAccumulator.cpp:240`), so a descriptor set is never reused while
   its frame may still be executing — this requirement is unchanged.
5. **Resize/`invalidateHistory()`:** both paths must leave `_hasHistory =
   false` so the next frame writes history with full weight instead of
   blending against recreated/undefined content. `resize()` already recreates
   the images; confirm the scalar state reset happens in
   `createHistoryImages()` or `cleanupImages()`.
6. **First frame after build:** identical to resize — both history images are
   pre-transitioned and `_hasHistory == false`.

---

## 5. Expected effects and non-effects

**Fixed by this change:**

- Alternating-frame flicker/pulsing of accumulated AO, RT reflections and RT
  GI (root cause removed: one noise sequence feeds one history chain).
- Progressive accumulation mode converges at full frame rate.
- Reprojection rejection (depth/normal thresholds) now compares against the
  immediately previous frame — fewer ghosting/rejection artifacts during
  slow camera motion.

**Not addressed by this change (out of scope, previously documented):**

- White-noise sampling (no blue noise) in the RT passes.
- Missing FSR history reset on camera cuts.
- Camera-only motion vectors (dynamic objects).

**Resource savings:** per accumulator instance, history images drop from
`2 × numImages()` to 2, prev-depth and prev-normal images from `numImages()`
to 1 each. With 6 instances (2 layers × 3 accumulators) and 2 frames in
flight, that is a 50 % reduction in temporal-pass image memory.

---

## 6. Verification surface

No automated tests exist in the project; verification is visual +
validation-layer based:

1. Build with `VK_LAYER_KHRONOS_validation` enabled and confirm zero layout /
   synchronization errors, including across window resize and render-scale
   changes (both recreate the history images).
2. Run an RT-enabled example or `model_edit`; with a **static camera** and
   Progressive mode, confirm the accumulated result converges monotonically
   without alternating between two states.
3. Enable the debug views `TemporalAccumulatedAO`,
   `TemporalAccumulatedReflections` and `DenoisedGI`
   (`DeferredLayer::resolveDebugSource`, `DeferredLayer.cpp:62-81`) and
   confirm the accumulated buffers are stable frame-to-frame.
4. Exercise camera motion (Interactive mode) to confirm reprojection
   rejection still works and no new ghosting appears.
5. Exercise `invalidateHistory()` (if exposed through the UI/settings) and
   confirm clean restart of accumulation.
