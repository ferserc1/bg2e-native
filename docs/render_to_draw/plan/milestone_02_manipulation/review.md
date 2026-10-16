# M02 code-versus-remaining-plan review

Date: 2026-10-16. [Progress](../progress.md) · [Review procedure](../review-protocol.md) · [Architecture decisions](../architecture-decisions.md)

## Baseline and acceptance

The source revision at review entry and delivery is `7b217bcb6d9a3473ea522dd258d8d87b87150ddb`; M02 and prior M01 work remain uncommitted in the inherited working tree. The review preserved that tree. The M02-07 product is [examples/draw/11_manipulation](../../../../examples/draw/11_manipulation/src/main.cpp), registered in `examples/CMakeLists.txt` as `draw_manipulation` with its own `bundle_app_sdl` target. This review performed no build, shader compilation, executable launch or runtime/device check.

At review entry `git status --short` reported 73 inherited worktree entries; they
span the ongoing M01/M02 source, probes and planning work and are not presented
as changes created by this review. The host is Darwin arm64; installed Vulkan
SDK/MoltenVK versions and current backend execution were not supplied.

Source map inspected: [SelectionManager](../../../../lib/include/bg2e/manipulation/SelectionManager.hpp) and [draw picking/input implementation](../../../../lib/src/bg2e/manipulation/DrawPicking.cpp); [GizmoComponent](../../../../lib/include/bg2e/manipulation/GizmoComponent.hpp) and [draw overlay](../../../../lib/src/bg2e/manipulation/DrawOverlays.cpp); [MaterialModifier](../../../../lib/include/bg2e/utils/MaterialModifier.hpp) and [draw patch implementation](../../../../lib/src/bg2e/utils/MaterialModifier.cpp); [DrawMaterialEditor](../../../../lib/include/bg2e/ui/DrawMaterialEditor.hpp) and [queued editor](../../../../lib/src/bg2e/ui/DrawMaterialEditor.cpp); [draw viewpoint API](../../../../lib/include/bg2e/utils/ViewpointAnalyzer.hpp), [analyzer](../../../../lib/src/bg2e/utils/ViewpointAnalyzer.cpp) and [camera helper](../../../../lib/src/bg2e/utils/SceneCameraUtils.cpp); [Drawable lifecycle](../../../../lib/src/bg2e/draw/Drawable.cpp); and the registered [manipulation example](../../../../examples/draw/11_manipulation/src/main.cpp).

M02-01 through M02-06 were accepted by project-lead continuation; M02-01 has a reported macOS Metal/Linux Vulkan validation boundary, while later backend/scenario coverage was not specified. M02-07 is accepted by the lead's current continuation instruction under the shared protocol; this assumes compilation/checks succeeded without failures but does not establish any specific runtime case or backend. Agent work for this review is source/plan inspection only. The project lead remains responsible for Vulkan/Metal, no-RT, UI and production regression behavior.

## Planned versus actual contracts

| Area / classification | Actual API, ownership and behavior | Remaining impact |
|---|---|---|
| Execution modes and scene components — equivalent refinement | `manipulation::SelectionManager` and `GizmoComponent` have production/draw overloads and fixed `ToolExecutionMode`. `SelectionItem` keeps weak node/component/resource references, mode, submesh, kind and global `SelectableComponent` identity. `refreshSelection()` prunes replaced/detached/stale references. Gizmo components own instance-specific draw resources; they attach to scene nodes and stay outside regular render-queue geometry. | M06/M07 use typed mode/accessors and must invalidate selection on replacement. M09 retires production overloads only after replacement coverage. |
| Picking and identity — equivalent refinement | Draw picking in `DrawPicking.cpp` extracts the normal `RenderQueue` for visible scene geometry, renders submission-local RGBA8 IDs and synchronously reads them back. It maps retained queue IDs to weak typed selection items; IDs are distinct from persistent selectable IDs and TLAS order. The draw path uses D32 for visibility and no material alpha test. Input coordinates are top-left logical viewport coordinates scaled by `setPickPixelScale`. | M03/M04 must not reuse pick IDs as stable scene or TLAS identities. M06/M07 must account for synchronous stopped-boundary pick latency and geometric cutout behavior in parity checks. |
| Overlay and gizmo lifecycle — equivalent refinement | `RendererConfiguration::overlays` retains optional shared `OverlayPass` objects. `GizmoAndSelectionRenderer` supplies the manipulation overlay through `MeshOverlayPass`; renderer callbacks expose borrowed output/camera/queue context after final processing. Per-slot draw owners survive submission. `GizmoComponent` resources require explicit cleanup while Engine/device remain alive. | M05 must define whether output images include UI/overlays; M06/M07 teardown and replacement paths must release tools before Engine. M09 retirement includes legacy and draw UI/native registration boundaries. |
| Material patching — equivalent refinement | `draw::Drawable::materialDefinition()` is stored CPU state; `material()` aliases live attributes after load. Draw `MaterialModifier` applies specified properties independently to both, or live-only through `draw::MaterialBase`; indexed/name/exact-group/regex-search overloads preserve unrelated values. Texture realization uses `updateTextures`; exceptions and multi-target edits are nontransactional. The utility owns no scene/renderer invalidation policy. | M03 histories need app notifications and reset at queued edit boundaries. M04 bake generation must invalidate on relevant source edits. M06/M07 retain weak target and generation checks, redraw/reset and unsaved-state handling. |
| Widgets and reflection — intentional scope boundary | `DrawMaterialEditor` queues weak-target, renderer-generation and binding-revision checked partial edits; `DrawTextureWidgets` uses UI-session `TexturePreview` snapshots and cleanup. Generic `ComponentInspector`/raw reflected node setters are not made draw-safe by M02. Existing production editor widgets remain intact. | M06/M07 must implement safe node/component/property queues rather than attach the production inspector directly. M09 cannot remove old widgets until each real caller is migrated. |
| Viewpoint tools — equivalent refinement | `utils::ViewpointAnalyzer(draw::Engine*)` can analyze a draw scene synchronously at a stopped producer boundary; it maps candidate IDs through `analyzedSubmeshes()` and reads full D32 images through the typed GPU API. `applyViewpointToOrbitCamera` has a draw candidate overload; production analyzer/helper remain unchanged. | M06/M07 must call analysis only as an explicit stopped-boundary action, preserve app scoring/presets and handle empty/failure cases without committing partial camera/document state. M05 image averaging must not invoke or count analysis work as a scene sample. |
| Cloning and rare edits — existing contract with explicit limits | Loaded `draw::Drawable::clone()` copies uploaded mesh data and independently clones live materials/textures; `materialDefinition()` enables partial stored/live mutation without whole-material overwrite. `reload()` is a synchronized mesh/BLAS rebuild path, while ordinary material texture replacement uses the existing texture-update path. Cleanup snapshots live state into definitions, so an arbitrary CPU/live divergence is not promised to survive cleanup or a later clone. | M04 UV2 must use a stopped reload/rebuild and refresh external queues/bindings. Same-topology UV reload does not necessarily change `SelectableComponent` IDs; topology/CPU mesh replacement can. M04 still needs its own source/bake generation. M06/M07 complete-save/clone tests must verify current live state and explicitly handle generated GPU-only data; no step may infer export support from clone support. |
| Product parity and evidence — verification gap | The new target exercises selection, transform handles, MaterialModifier scalar/texture/UV/group/regex presets, clone edits, best-view, deletion, logical viewport/pick scale and `--no-rt`. Static checks and a lead checklist exist. This review did not observe the executable, rendering, packaging, asynchronous texture updates, or either backend. | M02 runtime/product exit checks remain lead-owned. Downstream parity statements must distinguish source/API availability from observed behavior. |

No deep-clone or partial-patch guarantee was weakened. The intentional M02 exclusions remain generic draw-safe component/property inspection, full editor parity, UV/baking, advanced RT, asynchronous import and headless output. No new API or architecture decision is required to enter the next M02 documentation step.

## Verification ledger

| Evidence | Result |
|---|---|
| Lead continuation through M02-01 | Accepted under protocol; lead previously reported checks through that point on macOS Metal and Linux Vulkan. No finer case matrix was supplied. |
| Lead continuation M02-02 through M02-06 | Accepted under protocol; compilation/checks assumed successful. Backend/scenario coverage unspecified. |
| Lead continuation M02-07 | Accepted under protocol on 2026-10-16; compilation/checks assumed successful. Backend/scenario coverage unspecified. |
| Agent M02-08 static review | Compared actual public headers, implementations, plan references, `draw_manipulation` caller, M01 review and all M03–M10 pending summaries/steps. Scoped Markdown links and whitespace were checked. No compile, shader, executable, runtime, image or native SDK check. |
| Product checklist | Picking, transform capture, overlays, scalar/texture edits, group/regex, clone behavior, resize/DPI, optional viewpoint, no-RT and production regressions remain for the lead to exercise on available targets. |

The inherited checkout is Darwin arm64, but installed SDK versions and actual M02 backend/runtime results are not recorded here. Do not infer platform coverage from the host or from a continuation that did not specify it.

## Pending-plan impact and edited files

The following are concrete updates, not claims that future work has been implemented. M03's design checkpoint remains owned by the project lead; M02 does not decide advanced RT material addressing, transparency policy, or the native Metal execution model.

| Pending milestone | Impact from actual M02 contract | Updated owning plan files |
|---|---|---|
| M03 — advanced RT | Use distinct per-snapshot pick IDs and dense TLAS order; extend app edit notifications/reset beyond `resetUpscaler`; keep no-RT and neutral fallback behavior. The explicit backend/table decision checkpoint is unchanged. | `milestone_03_ray_tracing/summary.md`, `step_01_rt-design-checkpoint.md`, `step_07_temporal.md`, `step_09_existing-example-integration.md` |
| M04 — baking | UV2/topology changes invalidate render queues and bake generations; use stopped `Drawable::reload()` and rebind. Selection IDs may persist across a same-topology reload; bake generation must be separate. Keep bake accumulation separate from render histories; camera picking/analyzer readback is not a bake surface. | `milestone_04_lightmaps/summary.md`, `step_02_uv2-surface.md`, `step_03_integrated-context.md`, `step_05_uv-preview.md` |
| M05 — offscreen | Scene-frame averages must define overlay/UI inclusion. Synchronous picking/viewpoint work is not a rendered sample. Reuse the existing synchronous readback layer without treating RGBA8/D32 pick paths as export conversion. | `milestone_05_offscreen/summary.md`, `step_03_accumulation-settings.md`, `step_05_readback-export.md` |
| M06 — model editor | Compose DrawMaterialEditor/DrawTextureWidgets and typed selection. Generic inspector mutation still needs app-owned queued weak-target/scene-generation handling. Viewpoint analysis is explicit, synchronous, and failure-safe; UV2 uses a stopped reload boundary. | `milestone_06_model_edit/summary.md`, `step_03_editor-tools.md`, `step_04_viewpoint-camera.md`, `step_05_editor-baking.md` |
| M07 — composer | Apply the same safe edit/reset/unsaved-state contract to raw scene/property changes; async import needs independent scene/job validity and must invalidate selection/editor targets when replacing nodes/resources. | `milestone_07_bg2e_composer/summary.md`, `step_03_scene-editing.md`, `step_04_async-import-service.md` |
| M08 — standalone baker | Scene assembly and UV2 must regenerate bake/selection identities after reload; no viewport gizmo or overlay enters traceable/export data. Complete resource export remains separate from GPU clone support. | `milestone_08_lightmap_generator/summary.md`, `step_02_scene-assembly.md`, `step_03_batch-output.md` |
| M09 — cleanup | Inventory the new draw widget/session preview, `OverlayPass`, dual-mode manipulation and synchronous readback paths; remove production branches only after replacement evidence, and release editor/gizmo resources before UI/Engine teardown. | `milestone_09_cleanup/summary.md`, `step_01_retirement-manifest.md`, `step_03_shared-api-cleanup.md` |
| M10 — documentation audit | Reconcile typed selection identity, stopped-boundary analyzer/readback, partial CPU/live edits, session preview ownership, output overlays and evidence boundaries against final source. | `milestone_10_documentation/summary.md`, `step_02_api-reference.md`, `step_03_tutorials.md` |

The M03–M10 changes are listed at their end for traceability. Completed M01/M02 history was not rewritten except for progress acceptance bookkeeping. The root order and unresolved architecture decisions are unchanged.

## Entry condition for M03 and M02 documentation handoff

M02's technical review is delivered and awaits project-lead acceptance. The required next step is [M02-09 cumulative documentation](step_09_documentation.md); M03 remains gated on accepted M02-09 and its own backend design checkpoint. No M03 implementation or advanced RT decision is authorized by this review.

## Post-review compilation correction — 2026-10-16

After this review was delivered, the project lead reported clang errors in
`draw_manipulation`: an invalid base delegate constructor and an unavailable
`weak_from_this()`. This failure report supersedes the earlier assumption of
successful example compilation; no passing correction build is recorded yet.
The lead also required application examples to access SDL functionality through
engine wrappers.

The example now passes `std::make_unique<draw::RendererDeferred>()` together with
its overlay configuration to `DefaultRenderLoopDelegate`. `UiDelegate` inherits
`std::enable_shared_from_this<UiDelegate>`; it is created by `make_shared` before
initialization, so its cleanup callback can retain a weak UI reference without a
cycle. Ctrl queries use the existing `app::Keyboard::controlPressed()` wrapper.
Both window queries now use `app::MainLoop::windowSize()`, a new logical-size
accessor populated before initialization callbacks and updated on resize. Its
cached dimensions reset on normal or exceptional run exit. SDL remains inside
the engine; the example has no direct SDL include, call or native window access.

This adds an application wrapper contract to the source map above. Downstream
editor/composer input and DPI integration should use engine wrappers as required
by M02-07. The M03–M10 technical contracts otherwise remain unchanged. Static
inspection covered constructor signatures, shared/weak ownership, initial and
resize ordering, exception cleanup and native API boundaries. Compilation and
resize/DPI/Ctrl-click/teardown runtime checks remain with the project lead.
M02-08 acceptance and M02-09 entry remain gated on this correction's validation.

## Fixed picking scale correction — 2026-10-16

The lead corrected the reported dimensions to zero. Source inspection confirms
that `RenderLoop::frame()` invokes the delegate's first `prepareFrame()` before
`surfaceChanged()` provides the renderer's output extent. Reading configuration
at that point was an example lifecycle error, not uninitialized `Size2D` storage.

The example now uses one fixed `RenderScale = 0.7f`: resolution scaling is enabled
in `makeRendererConfiguration()` and picking receives the same positive scale in
`init()`. Comments cross-reference these functions and explain the independent
logical-coordinate pick image versus physical render pixels on Retina. The
per-frame `refreshPickScale()` configuration query was removed. Static inspection
only; startup, resize/DPI and selection/drag checks remain with the lead.

## M02-09 cumulative documentation handoff — 2026-10-16

The project lead reports the camera/manipulation issue is solved and requested
the documentation step, with Vulkan testing still outstanding. The temporary
`[camera-debug]` trace code and its triggers/state were removed from the
manipulation example; input routing and viewpoint behavior were left intact.

Maintained references/tutorial/application guidance are recorded in
[documentation.md](documentation.md). New canonical references cover
`manipulation` and `utils`; UI and input documents distinguish production
widgets from draw wrappers and explain explicit event forwarding. The draw
tutorial and applications index link the registered `draw_manipulation`
example. Static links, anchors, header/source/target paths and whitespace were
checked. No build, shader compilation or runtime/backend test was run. Vulkan
remains the project lead's outstanding M02 validation item; M03 is not started
until the documentation step and this Vulkan gate are accepted/completed.

### Tutorial authoring correction

The lead requested an instructional tutorial rather than an overview or a full
source listing. Tutorial 11 was rewritten as ordered construction steps with
focused code, explanations and concrete recipes for repeated fixtures/controls.
The application-input guide now links to its new event-routing section. The
residual `--no-rt` configuration was also removed in accordance with the earlier
lead instruction. M02-09 remains delivered for acceptance, with Vulkan runtime
validation outstanding. Source/API and Markdown checks are static only.
