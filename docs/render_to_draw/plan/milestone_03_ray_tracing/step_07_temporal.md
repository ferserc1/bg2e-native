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
