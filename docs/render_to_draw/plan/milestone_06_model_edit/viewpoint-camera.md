# M06-04 viewpoint and camera implementation handoff

Delivery date: 2026-10-19. M06-03 was accepted by project lead continuation
instruction; backend/scenario coverage was unspecified. Starting/finishing HEAD
remains `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`. Inherited and staged work was
preserved. No commit/reset/staging, CMake change, build/configuration, shader tool,
application run or probe execution was performed.

## Camera actions

`StageScene::frameNode` uses `scene::BoundingBox` world bounds, the current
vertical camera FOV and the bounding sphere radius to update the existing
same-node `OrbitCameraComponent` center/distance. Degenerate or non-finite bounds
return failure before changing the pose. The existing fixed-pose Reset camera
remains separate.

`AppDelegate::frameSelection` and `analyzeBestView` are renderer updates queued
before frame acquisition. They capture weak document node/delegate references,
renderer generation and document generation, then revalidate live document
membership before acting. With no current document selection, the document root
is used. Bounds framing does not change document dirty state or projection
settings.

Best-view analysis waits for device idle, then calls the draw-backed
`utils::ViewpointAnalyzer` on the target document subtree. It samples the existing 18-yaw by 4-pitch defaults within a maximum 48-pixel
image extent preserving the active viewport aspect. It uses the current vertical
FOV, midpoint target and default 1.1 distance scale. It excludes the editor floor, fill light, environment and
camera because those are outside the target document subtree. `bestSample` selects
the result. Pixel-array positions map to the live analyzer's `analyzedSubmeshes`
entries by 1-based identifier; the UI reports candidate count/index, score,
visible pixel/submesh coverage and up to three most visible submeshes. Only after
nonempty visible geometry is confirmed is the candidate applied via
`applyViewpointToOrbitCamera`, which preserves projection settings. Empty/error
paths preserve camera pose and report diagnostics. Analyzer resources are retained
by StageScene and released before document draw resources and Engine teardown.

## Validation handoff

Static inspection covered declarations/call sites, weak targets/generation/member
checks, selected/document-root scope, finite bounds before camera mutation,
identifier mapping and UI-reachable status. Documentation paths and fences plus
`git diff --check` were checked. No compiler, executable, C++ probe or GPU/runtime
validation was run.

Project-lead checks: frame tiny/large/off-center/transformed models at several
FOVs and viewport aspect ratios; test empty/disabled/hidden geometry and document
root versus selected node; verify analyzer score and top submesh pixel reporting;
force analyzer/readback errors and confirm unchanged camera pose; test no-RT
availability, document replacement before queued actions, close/reopen cleanup,
and confirm no stage fixture enters measured geometry. M06-04 is delivered
awaiting these checks. M06-05 remains UV2 and integrated lightmap work.
