# M02-03 — Selection highlights, public overlay components and gizmos

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M02-01/M02-02.

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
3. Draw after reconstructed final output at output resolution with original unjittered camera matrices and explicit overlay depth policy. RendererDeferred currently keeps camera/targets private: add the smallest public borrowed surface/camera/overlay attachment contract required here, with acquired-slot/completion lifetime, rather than accessing private fields or copying the renderer. Tool geometry is excluded from normal lighting/RT extraction.
4. Expose configuration/shader/component replacements rather than hide a new private graphics renderer; preserve ordinary manipulation accessors.

## Verification and exit conditions

- Highlight and handles stay stable/sharp under FSR/internal scaling; transform gizmo capture prevents camera input and applies correct transforms.
- No native Vulkan implementations live in manipulation; no-RT mode retains all editor graphics.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06/M07 integrate these optional objects. M05 headless paths must omit overlays without modifying the core renderer.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M02-01 actual handoff

M02-01 adds draw init(engine) and ToolExecutionMode to GizmoAndSelectionRenderer, but no draw recording yet. Extend this public optional tool with the planned GPU surface/camera contract. GizmoComponent exposes manipulationMode, transformDrawDrawable, synchronizeResources and cleanupDrawResources, with instance-owned draw caches and weak safe callbacks. Preserve unlit axis colors, no tracing geometry, typed production paths and stopped-boundary resource release.

## M02-02 actual picking handoff

See [picking.md](picking.md) for synchronous submission-local IDs, typed selection,
unjittered/top-left viewport coordinates, setPickPixelScale and stopped-producer
requirements. Reuse RGBA8 geometric picking; no depth readback was added.
Route input at a safe boundary and refresh DPI scale on window changes.
