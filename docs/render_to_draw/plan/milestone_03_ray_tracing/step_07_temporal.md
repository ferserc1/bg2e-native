# M03-07 — Rendered-view histories and temporal accumulation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M03-04–M03-06 and M01 runtime delegate sequence.

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
3. Use completion-safe mutable shader/output/history ownership, not rings of mesh/texture assets. Preserve explicit history formats and previous camera/jitter conventions.
4. Expose accumulation controls and replacement configuration. Define bounded convergence/frame-count behavior for later headless jobs; record whether camera-only motion suffices or moving-object vectors need expansion.

## Verification and exit conditions

- Static results converge; resize/edit/camera discontinuity resets correctly; UI-only presentation and paused frames do not advance sample chronology.
- AO, GI and reflection histories cannot overwrite each other and FSR switching resets its own SDK state consistently.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M04 UV accumulation remains a distinct bake signal, M05 selects actual rendered accumulation frames, and apps must notify the real reset paths.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
