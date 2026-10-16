# M02-09 — Cumulative documentation handoff

Date: 2026-10-16. [Progress](../progress.md) · [M02 review](review.md) ·
[Step](step_09_documentation.md) · [Documentation policy](../documentation-policy.md)

## Coverage and dispositions

| Maintained page | Disposition | Actual source/product mapping |
|---|---|---|
| `doc/api/manipulation/index.md`, `reference.md` | Added | `SelectionManager.hpp`, `GizmoComponent.hpp`, `GizmoAndSelectionRenderer.hpp`; `DrawPicking.cpp`, `DrawOverlays.cpp`, `SelectionManager.cpp`. Covers execution modes, weak selection identity, draw pick scale/readback, capture and cleanup. |
| `doc/api/utils/index.md`, `MaterialModifier.md`, `ViewpointAnalyzer.md`, `SceneCameraUtils.md` | Added | `MaterialModifier.hpp/.cpp`, `ViewpointAnalyzer.hpp/.cpp`, `SceneCameraUtils.hpp/.cpp`. Documents partial CPU/live patches, nontransactional texture edits, synchronous analyzer lifecycle and projection-preserving camera application. |
| `doc/api/ui/Material_Editors.md`, `TextureWidgets.md`, `DrawTextureWidgets.md`, `index.md`, `UserInterface.md` | Updated/added | Production editor/widget distinction retained; `DrawMaterialEditor.hpp/.cpp`, `DrawTextureWidgets.hpp/.cpp`, `TexturePreview.hpp/.cpp`. Documents queued draw edits and separate preview ownership without claiming general editor parity. |
| `doc/api/app/Application_and_input.md`, `doc/input_delegate.md` | Updated | `InputDelegate`, `InputVisitor`, and example `ManipulationInput` call sites. Clarifies that input callbacks are not automatically delivered to scene components. |
| `doc/tutorials/draw/11_manipulation.md`, `doc/tutorials/draw/index.md` | Added/updated | Registered `examples/draw/11_manipulation` target. Covers scale initialization, camera component placement, left-click/drag tolerance, right-pan, middle-zoom, selection capture, patching and best-view action. |
| `doc/api/draw/index.md` | Updated | Links the draw manipulation tutorial and API entry point while retaining render/draw coexistence language. |
| `doc/apps/draw_manipulation.md`, `doc/apps/index.md` | Added/updated | `examples/CMakeLists.txt`, target `draw_manipulation`, example resources and common deployment guidance in `draw_deferred_renderer.md`. No separate CLI options or production application are claimed. |
| Other maintained M01 foundation/API pages | Retained | Existing accurate pages were checked for affected navigation and statements. M01 deferred renderer, GPU, application build, and packaging contracts remain unchanged. No page was retired. |

The new tutorial describes current code, including the intentionally broad
teaching sample and its extra material presets. It does not claim the sample is
a minimal production editor. Future RT, baking, copied apps and transparency
work remain outside M02 user documentation.

## Tutorial correction after lead feedback

The lead rejected the overview-style tutorial and the subsequent complete-file
listing. Tutorial 11 now teaches construction from an empty source file through
26 ordered steps, following tutorial 10's explanatory approach. Each step adds
a responsibility with a focused fragment or a precise implementation recipe;
there is no full example listing. It covers scene construction, ownership,
coordinates, click/drag/capture routing, queued UI edits, analysis, deletion,
cleanup, registration and platform build instructions. Repetitive fixtures and
controls are specified with concrete values and APIs instead of duplicating
whole code blocks. The incoming input-guide anchor was updated accordingly.

The earlier instruction to remove command-line configuration was also completed:
the residual `--no-rt` parser, UI message and configuration state were removed
from the example. The default tracing/shadow policy stays enabled through the
existing resource defaults. No build or runtime validation was performed.

## Validation and limits

- Checked touched Markdown links and anchors, API header paths, example source
  paths and target registration statically; checked camera-debug identifiers
  are absent from the example after removing the temporary trace code.
- `git diff --check` was run on the documentation/source changes.
- No CMake configure/build, shader compilation, executable, Vulkan or Metal
  runtime validation was run in this documentation step. The project lead
  reports the camera/manipulation issue is solved; Vulkan testing remains
  outstanding before M02 can be closed. No backend-specific result beyond that
  report is inferred here.

## Remaining ownership

The project lead owns the outstanding Vulkan runtime check and final M02
acceptance. Later milestones must update these canonical pages when their actual
contracts change; M10 remains the cumulative inventory and navigation audit.
M03 work is gated on acceptance of this documentation step and completion of
the outstanding M02 Vulkan check.
