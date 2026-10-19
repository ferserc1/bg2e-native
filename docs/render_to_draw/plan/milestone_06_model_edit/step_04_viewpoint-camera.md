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

M02-06 delivers `utils::ViewpointAnalyzer(draw::Engine*)`, draw-scene overloads, candidate/statistic types and a typed D32 GPU readback. Framing analysis is synchronous and requires stopped scene/GPU producers; use the draw overload of `applyViewpointToOrbitCamera` to update candidate projection data. The production overload and analyzer remain unchanged.

M02-08 confirms this is an explicit, synchronous editor action, not suitable for
UI preparation or an asynchronous worker without an additional ownership/job
design. Map returned IDs through that result's `analyzedSubmeshes()` only. A
failed or empty analysis must leave camera, document and render settings unchanged;
do not serialize temporary analyzer configuration as user settings. See
[M02 review](../milestone_02_manipulation/review.md).

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


## M06-01 camera baseline

StageScene now uses one node containing transform, OrbitCameraComponent and camera,
with left orbit/right pan/wheel zoom. Its reset is a fixed pose (15/30 degree
rotation, distance 8) queued through AppDelegate; it is not ViewpointAnalyzer.
Add the actual best-view/model framing here, preserving the single-node pose
contract and the [inventory](application-inventory.md) camera-control requirements.


## M06-03 concrete stage/tool baseline

Use [the editor tool handoff](editor-tools.md). StageScene now owns a Scene catalog
with the same-node editor camera and a separate Document root. AppDelegate queues
input/picking/manipulation before acquisition, converts logical pointer coordinates
to the camera viewport, and guards edits with document generation plus weak target
membership. The environment panel exposes stopped stage camera intrinsics; fixed
reset still uses an orbit pose. Neither feature supplies automatic bounds framing
or viewpoint analysis. Add analysis/framing to these owners and queue/revalidate
its result without changing document/renderer state on failed or empty analysis.

## M06-04 delivered implementation

Implemented in `apps/draw/model_edit`: `StageScene::frameNode` frames world bounds
using the active camera's narrower horizontal/vertical FOV; the toolbar and
properties panel expose **Frame selection** and **Analyze best view**. Analysis
uses the draw `utils::ViewpointAnalyzer` on the selected document node or document
root, with 72 candidates at up to 48 pixels per longest image dimension, preserving the active viewport aspect, and current vertical FOV. It runs as a queued
stopped renderer update after `waitIdle`, then applies only a visible best candidate
through `applyViewpointToOrbitCamera`. Pixel counts use that analyzer's 1-based
`analyzedSubmeshes()` table. Weak target/delegate, renderer generation, document
generation and document membership are checked before execution. Candidate count,
winning index/score, coverage and three top submeshes are displayed. Empty/error
analysis leaves the prior pose unchanged. StageScene owns analyzer resources and
releases them before draw scene teardown.

Implementation evidence and project-lead checks are recorded in
[viewpoint-camera.md](viewpoint-camera.md). Static checks are not compiler or
runtime verification; the exit conditions above remain for project-lead validation.
