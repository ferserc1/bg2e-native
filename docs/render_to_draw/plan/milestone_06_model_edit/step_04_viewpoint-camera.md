# M06-04 — Utils viewpoint analysis and camera helpers in editor

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M06-03 and M02-06.

## Scope and deliverable

- `apps/model_edit/src/StageScene.cpp`
- `lib/include/bg2e/utils/ViewpointAnalyzer.hpp`
- `lib/include/bg2e/utils/SceneCameraUtils.hpp`
- `apps/draw/model_edit/src/StageScene.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/model_edit/src/StageScene.cpp](../../../../apps/model_edit/src/StageScene.cpp), [lib/include/bg2e/utils/SceneCameraUtils.hpp](../../../../lib/include/bg2e/utils/SceneCameraUtils.hpp).

## Implementation work

1. Wire automatic framing/best-view workflows to the new utils analyzer/candidate types and new camera helper overloads.
2. Preserve existing application presets, bounds fitting, candidate scoring/extensibility and orbit-controller transforms, including camera translated below an orbit node.
3. Leave production render::ViewpointAnalyzer and old application call path unchanged until M09.
4. Use actual draw geometry/readback ownership and ensure analysis can run without modifying edited model or persistent render settings unexpectedly.

## Verification and exit conditions

- Automatic camera selection frames representative models sensibly and produces inspectable candidate statistics.
- Canceled/failed/empty analysis leaves camera/document state coherent; no legacy GPU buffers are used by the new app.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M07 adopts these helpers for composed scene/model framing; final documentation distinguishes neutral utility policy from its graphics measurement dependencies.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
