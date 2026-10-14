# M02-07 — Build examples/draw/manipulation and modifier validation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M02-01–M02-06.

## Scope and deliverable

- `examples/draw/deferred_renderer/src/main.cpp`
- `examples/draw/manipulation/src/main.cpp`
- `examples/draw/manipulation/CMakeLists.txt`
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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03 verifies both existing examples after effects integration. M06/M07 use the validated MaterialModifier/widget/selection APIs.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
