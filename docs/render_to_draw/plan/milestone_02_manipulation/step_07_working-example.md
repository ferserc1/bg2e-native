# M02-07 — Build examples/draw/11_manipulation and modifier validation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M02-01–M02-06.

## Scope and deliverable

- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/11_manipulation/src/main.cpp`
- `examples/draw/11_manipulation/CMakeLists.txt`
- `examples/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Derive the M01 scene and non-template delegate, adding selectable nodes/submeshes and optional overlay integration.
2. Provide explicit MaterialModifier presets/JSON patches applied to selected submeshes, including scalar, texture, alpha/UV and group/regex examples. Keep the example focused on manipulation and API validation.
3. Add controls for clone then modify independently, selection input capture and optional viewpoint measurement. Reuse settings/upscaling/shader libraries rather than duplicate engine code.
4. Register a unique executable target and packaged resources; preserve the original deferred example as a non-editor baseline.

## Verification and exit conditions

- Select and modify a material through the specified utility, including texture refresh and preservation of unrelated live state on both available backends.
- DPI/scale picking, additive/deselect/delete, gizmo transform and cloned GPU independence have recorded results; no-RT rendering stays functional.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03 verifies both existing examples after effects integration. M06/M07 use the validated MaterialModifier/widget/selection APIs.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-15 scene product to derive from

Reuse the actual examples/draw/10_deferred_renderer product and [working-example.md](../milestone_01_deferred_renderer/working-example.md), not an assumed private renderer loop. Retain the shared facade/window, queued replacement/material edit generation checks, native-free app resampler and CountingGBuffer integration. The scene supplies OBJ/clones/glass/cutout/packed-UV fixtures; selection/gizmos remain this milestone's work.

## M02-01 actual handoff

Use the M02-01 typed selection/gizmo contracts in dual-manipulation.md. Call refreshSelection after scene/resource edits, release all instance-owned draw gizmo resources before Engine cleanup, and keep optional tools out of ordinary lighting/query geometry. Existing MainLoop safe callbacks use weak component/node lifetimes.
