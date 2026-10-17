# M05-02 — Windowless draw Engine and frame execution

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M05-01 and existing GPU OffscreenSurface.

## Scope and deliverable

- `lib/include/bg2e/draw/Engine.hpp`
- `lib/src/bg2e/draw/Engine.cpp`
- `lib/include/bg2e/gpu/OffscreenSurface.hpp`
- `lib/include/bg2e/draw/DefaultOffscreenApplicationDelegate.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/draw/Engine.hpp](../../../../lib/include/bg2e/draw/Engine.hpp), [lib/src/bg2e/draw/Engine.cpp](../../../../lib/src/bg2e/draw/Engine.cpp), [lib/include/bg2e/gpu/OffscreenSurface.hpp](../../../../lib/include/bg2e/gpu/OffscreenSurface.hpp).

## Implementation work

1. Add windowless initialization over GPU OffscreenSurface and generic surface/device context: the reviewed draw::Engine currently only exposes init(SDL_Window*, Backend&, EngineConfig) and WindowSurface* surface(). Retain explicit window-surface access for current callers, add generic/headless ownership, and adapt slot-count derivation without inventing a hidden SDL window.
2. Share resource/renderer services with windowed draw; define backend/Instance ownership and do not silently initialize a shared active Instance twice.
3. Implement a non-template offscreen renderer delegate using the same polymorphic Renderer and actual render-frame chronology, slot synchronization and explicit completion.
4. Disable editor overlays and force requested scene renders even though no presentation/dirty-scene UI is driving them. Define frozen-animation versus fixed-step time policy.

## Verification and exit conditions

- Headless deferred rendering initializes with no SDL window/UI, and windowed examples remain operational after surface accessor changes.
- Frames submit/complete safely with correct slots and cleanup on creation/render errors; no-RT devices use ordinary rendering.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M05-03 defines accumulated-frame semantics; M08 uses the headless context but not a camera renderer to obtain bake resources.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-05 lifecycle handoff

Reuse M01-05's non-template renderer ownership and synchronized initialize/attach/resize/update/record/cleanup contract. Headless scene submissions use sceneFrameNumber/sceneDeltaSeconds; do not infer scene history from presentation count or introduce a second incompatible FrameContext. Follow [runtime-renderer.md](../milestone_01_deferred_renderer/runtime-renderer.md) for queued replacement and stale settings-target invalidation.

## M02-03 optional overlay boundary

RendererConfiguration.overlays defaults empty. Offscreen/headless paths omit these
optional public post-final extensions and need no SelectionManager or manipulation
resources. See [overlay lifecycle](../milestone_02_manipulation/overlays.md).

## M03-06 reflection contract

[Reflection handoff](../milestone_03_ray_tracing/rt-reflections.md) defines output-relative quality independent of internal surfaces and unpremultiplied HDR hit radiance plus fractional confidence. Freeze scene/receiver/quality while advancing rendered sample keys for requested samples; accumulation must distinguish confidence from GI validity and retain IBL fallback for misses. Existing shared snapshot completion/ownership rules apply.

## M03-10 reviewed implementation requirement

Own a real completed-slot/serial sequence with stopped initialize/cleanup and ordered submissions, independent of a surface. Existing SMAA process(Device) is reusable, but does not create the windowless Engine/context. Keep Device and all borrowed signal/environment owners alive; no UI-only sequence increments.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
