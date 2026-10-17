# M04-06 — Build examples/draw/12_lightmaps with live GI disabled

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M04-01–M04-05.

## Scope and deliverable

- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/12_lightmaps/src/main.cpp`
- `examples/draw/12_lightmaps/CMakeLists.txt`
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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M05 derives from the non-bake deferred example; M06/M07 integrated bake UI uses this proven workflow, and M08 uses its validation but not its window scheduler.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-15 reusable product

Derive scene/application integration from examples/draw/10_deferred_renderer; see [working-example.md](../milestone_01_deferred_renderer/working-example.md). Its packed-channel fixture is material input/debug verification, not UV2 baking or a baked-lightmap implementation. Preserve shared delegate/settings/lifecycle integration when adding actual bake contexts.

## M03-10 reviewed implementation requirement

Start from the actual manipulation/deferred settings-window wiring, not assumed MainLoop auto-registration. When baked multipliers are applied explicitly disable live indirect AO/GI to avoid double lighting; decide reflection behavior separately. Verify chart isolation, full geometry reload and immutable snapshot ownership.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
