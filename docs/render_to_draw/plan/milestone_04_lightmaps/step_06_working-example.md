# M04-06 — Build examples/draw/lightmaps with live GI disabled

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M04-01–M04-05.

## Scope and deliverable

- `examples/draw/deferred_renderer/src/main.cpp`
- `examples/draw/lightmaps/src/main.cpp`
- `examples/draw/lightmaps/CMakeLists.txt`
- `examples/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Reuse the M01/M03 scene/delegate and disable live indirect RTAO/RTGI while preserving direct lights, environment/IBL, emission and baked-map composition.
2. Generate UV set 2 for suitable opaque target objects, safely rebuild them and prepare integrated bake jobs with progress/reset/cancel settings.
3. Apply completed maps to their original submesh/material definitions and live GPU materials with correct RGB multiplier/UV2 metadata; retain unbaked/transparent objects with explicit restrictions.
4. Add UV preview and configuration controls from actual bake objects. Register a runnable target/assets/shader libraries and report unsupported RT bake hardware without breaking scene display.

## Verification and exit conditions

- Baked indirect lighting is visible with live GI disabled; sample count/reset and cancellation have coherent progress/state.
- Target clone/geometry/material independence, UV coverage, viewport presentation and both output pixel formats have recorded results.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M05 derives from the non-bake deferred example; M06/M07 integrated bake UI uses this proven workflow, and M08 uses its validation but not its window scheduler.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
