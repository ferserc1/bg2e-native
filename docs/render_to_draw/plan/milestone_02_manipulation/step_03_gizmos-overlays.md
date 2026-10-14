# M02-03 — Selection highlights, public overlay components and gizmos

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M02-01/M02-02.

## Scope and deliverable

- `lib/src/bg2e/manipulation/GizmoAndSelectionRenderer.cpp`
- `lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp`
- `lib/include/bg2e/draw/pipeline/OverlayPass.hpp`
- `shaders/src/draw/selection_highlight/`
- `shaders/src/draw/gizmo/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/manipulation/GizmoAndSelectionRenderer.cpp](../../../../lib/src/bg2e/manipulation/GizmoAndSelectionRenderer.cpp), [lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp](../../../../lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp).

## Implementation work

1. Keep high-level GizmoAndSelectionRenderer in manipulation with draw initialization; record through public draw pipeline objects and GPU commands.
2. Implement depth-tested and depth-disabled variants, blending/line controls, type gizmos and transform-gizmo self-occlusion/last draw ordering.
3. Draw after final upscaling at output resolution with original unjittered camera matrices and dedicated/scissored depth policy. Tool geometry is excluded from normal lighting/RT extraction.
4. Expose configuration/shader/component replacements rather than hide a new private graphics renderer; preserve ordinary manipulation accessors.

## Verification and exit conditions

- Highlight and handles stay stable/sharp under FSR/internal scaling; transform gizmo capture prevents camera input and applies correct transforms.
- No native Vulkan implementations live in manipulation; no-RT mode retains all editor graphics.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M06/M07 integrate these optional objects. M05 headless paths must omit overlays without modifying the core renderer.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
