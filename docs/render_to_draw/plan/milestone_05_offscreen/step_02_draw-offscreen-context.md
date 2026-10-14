# M05-02 — Windowless draw Engine and frame execution

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M05-01 and existing GPU OffscreenSurface.

## Scope and deliverable

- `lib/include/bg2e/draw/Engine.hpp`
- `lib/src/bg2e/draw/Engine.cpp`
- `lib/include/bg2e/gpu/OffscreenSurface.hpp`
- `lib/include/bg2e/draw/DefaultOffscreenApplicationDelegate.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/draw/Engine.hpp](../../../../lib/include/bg2e/draw/Engine.hpp), [lib/src/bg2e/draw/Engine.cpp](../../../../lib/src/bg2e/draw/Engine.cpp), [lib/include/bg2e/gpu/OffscreenSurface.hpp](../../../../lib/include/bg2e/gpu/OffscreenSurface.hpp).

## Implementation work

1. Add windowless initialization over GPU OffscreenSurface and generic surface/device context. Preserve explicit windowSurface access for windowed UI/resize callers rather than assume all surfaces are WindowSurface.
2. Share resource/renderer services with windowed draw; define backend/Instance ownership and do not silently initialize a shared active Instance twice.
3. Implement a non-template offscreen renderer delegate using the same polymorphic Renderer and actual render-frame chronology, slot synchronization and explicit completion.
4. Disable editor overlays and force requested scene renders even though no presentation/dirty-scene UI is driving them. Define frozen-animation versus fixed-step time policy.

## Verification and exit conditions

- Headless deferred rendering initializes with no SDL window/UI, and windowed examples remain operational after surface accessor changes.
- Frames submit/complete safely with correct slots and cleanup on creation/render errors; no-RT devices use ordinary rendering.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M05-03 defines accumulated-frame semantics; M08 uses the headless context but not a camera renderer to obtain bake resources.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
