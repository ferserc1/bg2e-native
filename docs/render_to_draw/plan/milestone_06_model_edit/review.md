# M06 technical review — draw model editor

[Milestone](summary.md) · [Progress](../progress.md) · [Review procedure](../review-protocol.md) ·
[Product validation](product-validation.md) · [Documentation step](step_08_documentation.md)

## Baseline and product status

Review date: 2026-10-19. Repository HEAD at review start and finish:
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`. The application and M01–M05 work
being reviewed is present as staged, unstaged and untracked changes; no commit,
reset or staging operation was performed for this review. `git diff --stat HEAD`
reported 137 tracked changed files, 9,266 insertions and 268 deletions; untracked
files are additional. Changes include inherited M04/M05 work and prior M06 steps,
so this count is the working tree baseline, not an M06-only change count.

The M06 product sources are under `apps/draw/model_edit/`, target
`draw_model_edit`, app ID `org.bg2e.apps.draw.model-edit`, graphics preference
scope `draw.model_edit.graphics`. The production `apps/model_edit/` path remains
present. Product acceptance by continuation does not establish runtime/backend
coverage: M06-06 package and application scenarios were not executed by the agent
and remain explicitly unverified in [product-validation.md](product-validation.md).

## Planned and actual contracts

| Area | Actual contract and ownership | Classification / downstream effect |
|---|---|---|
| Application/runtime | `main.cpp` runs `app::MainLoop` with a draw `EngineConfig`; `AppDelegate` owns `draw::DefaultRenderLoopDelegate` and its `RendererDeferred`. No production Engine is created in the draw app. `bundle_app_sdl` registers the target and deploys paired engine shaders/assets. | Equivalent refinement. Keep both app identities and preference scopes through M08; prove coexistence before M09 retirement. |
| Scene/document | `StageScene` owns editor fixtures and camera separately from `Document::root`. Open/import builds a candidate before stopped adoption; save serializes the document root. Document generation is distinct from renderer/catalog generations. | Equivalent refinement. Composer requires scene-root generations and graph-level behavior beyond this one-model document. |
| DB load/save | `db::loadScene(path, draw::Engine&, progress)` realizes draw resources; `db::saveScene(root, draw::Engine&, path, progress)` snapshots them. `db::storeDrawableBg2(path, draw::Engine&, Node*)` is the one-model counterpart. `readSceneDocument`/`saveSceneReferences` remain engine-free and incomplete for GPU resources. `.bg2tex` v1 preserves exact mip/layer bytes and format; custom references require component hooks, external resources require explicit export contracts. | Equivalent refinement with format scope. M07 must use complete draw overloads for live resources and raw/schema IO only for their stated contracts; M08 still owns bake-specific encodings and batch publication. |
| Import formats | Draw BG2, OBJ/MTL and glTF realization overloads use an explicit draw context. Existing static glTF/import support and little-endian `.bg2tex` limits remain documented. | Intentional scope boundary. M07 must preserve current supported subset and report unsupported resources; no generic async importer was added to the engine. |
| Clone/edit | `scene::Node::clone` provides fresh IDs and draw `DrawableComponent` clones independent draw resources. App owns document-generation checks, safe queued mutation, dirty state, histories and transaction policy. `DrawMaterialEditor` exposes queued patches, typed texture settings, indirect mode and edit guard; it does not own application dirty state. | Equivalent refinement. M07 may reuse the reusable widgets/clone hooks but owns graph editing, app state, atomic multi-node edits and import generations. M09 retains production callers until replacement evidence. |
| UI/settings | `bg2e::ui::Workspace` and draw-specific material/texture widgets render through engine wrappers. Application owns panel layout, menus, settings identity and document integration. Live draw settings use the existing dynamic graphics facade with app-specific preference scope. | Equivalent refinement. No reusable Composer Workspace product is supplied by model_edit itself; M07 should compose public widgets and implement its own scene tree. |
| Manipulation | Draw `SelectionManager`/gizmo overlay support picking and target edits. `InputManager` tracks scene-owned mouse gestures across UI capture; app adds membership/generation guards. Shared production defaults remain. | Equivalent refinement plus common API behavior. M07 needs multi-node/scene-tree policies and production input regression checks; M09 waits for both callers. |
| Viewpoint | `utils::ViewpointAnalyzer(draw::Engine*)` borrows the Engine and scene resources and performs synchronous GPU-backed candidate analysis while producers are stopped. `StageScene` owns it and bounds framing uses the document subtree. | Equivalent refinement. M07 can reuse the helper, but must provide appropriate scene scope and serialize/guard long synchronous analysis. Backend behavior remains unverified. |
| UV2/baking | `app::Uv2SafeReload::regenerateDraw` queues independent replacement with weak node/delegate and resource identity checks. `bake::IntegratedBakerContext` borrows an initialized draw Engine, owns explicit source/environment state, allows one live baker and one prepared/in-flight frame, and requires caller `completeFrame`/`abandonFrame`. App owns source invalidation, frame submission/completion, cancellation and apply. | Intentional lifecycle boundary. M07 scene jobs must retire work before adoption and validate scene/job generations. M08 must implement an independent windowless scheduler; the integrated context is not a batch API. |
| Lightmap result/export | `LightmapPixels` and integrated baker output feed a live draw texture in the AO slot with linear UV2 and explicit baked AO/GI mode. Complete model/scene save materializes GPU-only image resources. | Equivalent refinement. M08 still needs typed batch preflight, map-file formats, collision/overwrite and protected-input transactions; M06 save alone does not provide them. |
| Distribution | Registered executable collection recursively finds app targets. Distribution installs executables, runtime libraries and shader/assets trees; native platform package/runtime behavior is still untested. | Verification gap. M09/M10 must validate final names/resources on supported generators and native packages. |

## Requirements and verification

Delivered source covers the feature rows reconciled in
[application-inventory.md](application-inventory.md): document IO, Workspace UI,
selection/material/geometry/environment tools, viewpoint analysis, UV2 and
integrated baking. Intentional exclusions remain: a generic editor service,
composer scene-tree/reflection, asynchronous import/server, a standalone bake
scheduler, batch output and production API retirement.

| Evidence owner | Result |
|---|---|
| Agent static review | Read actual public DB, draw UI, bake, UV2 reload and viewpoint declarations; reviewed app ownership/handoffs, packaging registration, M07–M10 pending summaries/steps, and plan/API boundaries. `git diff --check`, `git diff --cached --check`, Markdown links/fences passed for review outputs. |
| Project lead continuation | M06-06 accepted by continuation instruction. Under plan convention, prior delivery is accepted as compiled/checked without a reported failure. |
| Coverage not specified | No exact OS, Vulkan/Metal backend, SDK version, native dialog, package, runtime scenario, visual result or artifact was reported. Do not infer any of these from continuation. |
| Known remaining gate | The feature/package checklist in product-validation.md still needs native app and package execution evidence before M06 product claims can be treated as runtime-verified. |

No compilation, application, probe, shader tool or package script was run by the
agent. No core API depends on `apps/draw/model_edit`.

## Pending-plan impact

| Pending owner | Required adjustment from actual M06 contracts |
|---|---|
| M07-01, M07-03 | Reuse draw material widgets, Workspace building blocks, typed selection and camera/viewpoint helper. Keep app-owned graph/tree, scene generations, dirty state, async status and UI layout. |
| M07-02 | Use explicit-engine complete draw IO for live texture snapshots; preserve raw/schema distinction, component hooks, fresh node IDs, independent GPU clones, `.bg2tex` little-endian/draw-reader limits and importer subset. |
| M07-04 | M06 provides patterns, not a worker/server import service. Define distinct request and scene generations, worker-only CPU parse, stopped GPU realization/adoption, cancellation and stale completion handling. |
| M07-05 | Integrated baker is single-target and frame-lifecycle-bound. Composer owns multi-target scene roots, invalidation, result identity and complete-scene saves; do not put app-specific scheduling into reusable Engine APIs. |
| M08-01–03 | Reuse `LightmapBakeExecutor`, UV surface, accumulators/composition and generic DB resource save where applicable. Implement `StandaloneBakerContext`, `SceneAssembler`, `Batch`, complete preflight/output and protected publication as new bake owners. Do not reuse integrated window frame scheduling or M05 camera PNG output. |
| M09-01–06 | Preserve `model_edit` and production call paths through M08. Retirement manifest includes draw/production UI and DB overloads, input routing, app targets and both shader/runtime trees. M06 continuation is not runtime proof; require M06/M07/M08 product regression evidence before deleting production counterparts. |
| M10-04 | Audit current and final app target names, package scripts, paired draw/bake resources, Vulkan/Metal deployment, standalone/library update guidance and actual installed layout against source after M09. |

Affected pending files reviewed: M07 summary and steps 01–05; M08 summary and
steps 01–03; M09 summary and steps 01–06; M10 step 04. Their specific M06 handoff
notes are linked from those steps and from the [milestone summary](summary.md).

## Decisions and open gates

No user architecture decision is changed. DB stays generic for resources and
scene references; bake owns bake algorithms/jobs; applications own document
generations, UI composition, unsaved state and orchestration. Existing production
APIs stay in place while replacement callers mature. The missing runtime/package
evidence is a verification gap, not evidence of a feature reduction. Keep it visible
in M06 validation and the later production-regression gates.

M06-07 is delivered. The next required step is [M06-08 cumulative documentation](step_08_documentation.md);
M06-07 awaits project-lead validation under the execution protocol. M06 is not
closed and M07 is not authorized until M06-08 is accepted.

## Post-review correction: overlapping UV2

The project lead requested nonfatal overlap handling after an actual editor bake
failure. Bake/UV2 reload paths now use `UvAtlasOverlapPolicy::Warn`, which preserves
all triangles and reports overlap separately from structural failure. The editor
shows the warning persistently below UV2; legacy bake and standalone routes also
continue and emit diagnostics. Chart collision checking during atlas generation is
nonfatal. Numeric clipping now uses consistent zero-boundary classification and
translated polygon area. Regression sources were updated but not compiled/run.
These contracts supersede the earlier strict overlap requirement and must remain
in Composer and standalone consumers; M06-08 should retain the updated maintained
API and atlas architecture pages. Exact reproduction with the reported model and
Vulkan/Metal baking validation remain project-lead checks.

## M06-08 documentation handoff after accepted editor corrections

The project lead accepted the current editor work and explicitly reports correct
rendering with no lights. Continuation accepts M06-07 and the subsequent bake,
settings, focused reflection, gizmo and light-management corrections. The new
[documentation inventory](documentation.md) maps updated/retained maintained pages
to actual source through M06. The application guide, sequential editor tutorial,
settings/debug API references, complete IO and architecture reflect automatic
bake application with persistent preview, temporary/final resource ownership,
graphics-only preferences and nonpersistent debug views, light-only gizmos,
reflected numeric dragging and all-light removal/random placement.

Static link/anchor/fence/source/whitespace checks accompany this delivery. No
build, shader, package or application execution was performed by the agent.
Backend/package coverage beyond the reported zero-light behavior stays
unspecified. M06-08 awaits acceptance before M07-01; no Composer implementation
was started in this documentation step.
