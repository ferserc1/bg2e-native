# M07 technical review — Composer scene/import/bake contracts

The later [environment synchronization correction](environment-synchronization.md)
introduces explicit active Scene selection in Composer and removes its hidden
auxiliary Environment. Renderer, extraction and settings agree on selection;
inactive additions cannot steal it and removing the active component restores
renderer defaults. In-place colors/HDR editing and visible failure diagnostics
replace arbitrary-clone GPU generation/rollback. Lead validation is pending.

Direct-shadow follow-up extends the [capacity policy](tracing-capacity-16384.md)
to SoftShadowPass as well: its independent eight-pair table is no longer current.
The v2 query kernel uses exact device-clamped arrays and per-slot neutral alpha
instead of whole-scene shadow removal. Model Edit's working-shadow report and
Composer's failure prompted this correction; no native reproduction is claimed.
Texture-count/shader optimizations wait until API migration completes.

The subsequent [16384 tracing capacity correction](tracing-capacity-16384.md)
supersedes the old fixed profiles/native table ABI referenced by earlier reviews.
Current AO/GI/reflection native shader contracts are v2 with exact device-clamped
counts, Vulkan specialization and Metal argument-record buffers8/9. Keep object
materials independently owned and neutral-texture overflow warnings. Validation
is pending; this correction does not itself accept M07-07 or implement M07-08.

Subsequent failure/correction: [guarded loading and tracing investigation](loading-and-tracing-investigation.md)
records the new Open Scene Loader phases, joined MainLoop worker lifetime and the
macOS tracing table overflow. The latter is not a confirmed native
Metal RT pipeline defect. The lead selected warning plus no-texture fallback for
excess image/sampler pairs; the shared CPU/GLSL/Metal correction is delivered,
awaiting validation. Lead validation precedes M07-08;
earlier parity/availability assessments below do not imply that failure is fixed.

Subsequent lead-directed loading policy: unregistered JSON component types now
warn/skip and never abort graph loading. Schema/draw factory null handling and
the CPU reference probe are corrected; registered parser/resource failures remain
errors. This supersedes historical strict unknown-type wording below. Raw JSON
preserves unknown representations; typed graph save omits skipped components.

## Post-delivery correction: Composer panel/component parity

The lead subsequently rejected the inherited Environment Options and mixed Scene/
material panel layout. [UI parity correction](ui-parity-correction.md) supersedes
the earlier UI parity assessment: left Model Properties, right Scene tree/inspector,
no separate environment panel, and the production shared ComponentInspector with
queued add/remove/staged-draw callbacks instead of hard-coded component buttons.
Floor/fill fixtures start disabled and authored environment selection replaces the
fallback. Tool preparation no longer silently recreates removed Transforms.
Correction and this reconciled review await lead validation; no M07-08 work is
claimed. Earlier source/evidence entries below remain the original review history.

[Summary](summary.md) · [Product validation](product-validation.md) ·
[Progress](../progress.md) · [Documentation successor](step_08_documentation.md)

## Baseline and acceptance

Starting/finishing HEAD: `f7c2498cb4c29e27921c91809043dcf5fe4d963c`. Inherited
tracked and untracked M07 source/plan/UI corrections are preserved; no commit,
reset or staging is performed. The product is `apps/draw/bg2e_composer`, target
`draw_bg2e_composer`; production `apps/bg2e_composer` remains the behavior reference.

The project lead explicitly says the selection-crash correction is checked and
requests continuation. This accepts M07-06 including Chain controls and the
selection correction. Exact OS/backend/SDK, package, HTTP and numerical bake
coverage is unspecified. The positive selection correction result is recorded
specifically, not extrapolated into all-platform application parity.

This review performs nonexecuting source inspection. No configuration, compile,
shader compiler, parser/probe executable, HTTP request, application, GPU, install,
standalone generation or package script is run. M07 is not closed: M07-08 remains
the separate cumulative documentation step and its acceptance precedes M08.

## Source-backed contract comparison

| Responsibility | Actual API/source and lifecycle | Classification / consequence |
|---|---|---|
| Application ownership | Typed MainLoop/draw configuration, `DefaultRenderLoopDelegate` renderer ownership, StageScene/Document and independent UI/graphics/import identities; `bundle_app_sdl` target registration | Equivalent refinement. No direct production rendering call is introduced by Composer, but shared public headers still transitively include legacy types. This is coexistence, not completed retirement. |
| Root transfer | `Scene::releaseSceneRoot()` detaches the complete root and clears Scene caches. StageScene unwraps exactly one synthetic single-root level and retains multi-root containers | Equivalent refinement. Preserve authored hierarchy and ownership; M08 camera-less prefabs must not inherit Composer's enabled-camera Open restriction. |
| Component lookup/clone | `Node::getComponent` exact match then compatible subclass; `Node::clone` rejects null/shared/owned/sliced component clones; independent camera `Projection::clone`, controller gesture reset and draw CPU/GPU clones | Equivalent refinement with explicit extension contract. Registered custom types implement pure schema/reference/resource and exact clone hooks; typed graph IO is not raw unknown-JSON preservation. |
| Complete IO | `DocumentIo.cpp`/StageScene use explicit draw load/save and stopped adoption; full native image/mip/layer export, canonical scene paths, dirty Save/Discard/Cancel and main-file-last publication | Equivalent refinement. Reference-only saves are not complete exports. `.bg2tex` decoding and native byte-format limits remain; custom external resources require writer contracts. |
| Model export | Selected drawable nodes export mesh/material/joints without descendants or placement; each complete item commits independently | Intentional boundary. Earlier model exports survive a later item failure. This is not M08 whole-batch collision/protected-source/overwrite publication. |
| Tree/component editing | `SceneEditor.cpp`, SceneTree intent callbacks and queued weak document/member actions; active-camera/eight-light validation, local-transform reparent, typed node/submesh selection; Chain add/remove restored during M07-06 | Equivalent refinement. Known lightweight CPU components are staged; arbitrary GPU-owning component UI cloning is not promised. Helper gizmos are excluded through `DrawableComponent::isSceneGeometry`/`Node::drawable`. |
| UI table contract | `SelectableList::beginList` now returns bool; Composer opens its missing submesh table; both model editors, SubmeshSelector and GPUSelectionDialog honor false before items/endList | Corrected defect, not unfinished feature. The lead confirms the selection correction. No vendor/ImGui null-dereference workaround was added. Keep the contract and regression through cleanup. |
| Cameras/environment/tools | Only active authored controller receives viewport input; static cameras remain renderable; framing/viewpoint preserves projection and converts supported rigid/uniform parent poses; environment candidate failure preserves old state | Equivalent refinement with declared limits. Unsupported parent transforms and controller-less framing fail explicitly. Dirty/history/bake invalidation remains app-owned. |
| CPU import separation | Public `db::readGltf`/GltfDocument and `db::readObj`/ObjMesh, existing BG2 CPU data and parsed-data draw realization; prefab raw JSON on worker, factories/images/GPU realization main-thread/stopped | Equivalent refinement. Reuse parser entry points in M08, not app worker/HTTP classes. Referenced image files are not immutable filesystem snapshots; realization can still fail. |
| Async/service ownership | App-local SceneImporter owns one joinable CPU worker/16 waiting jobs. Job tokens, document generation, renderer identity and weak parent/reimport identities are separate; same renderer rebind reacquires current generation | Equivalent refinement. Cancellation is cooperative around third-party parser calls; synchronous realization is not interruptible. Renderer generation alone is not job validity. |
| HTTP/reimport publication | ImportServer binds localhost with one admitted request; canonical source reimport tracks weak wrapper/current parent. StageScene preflights excluding replaced subtree, restores graph/sibling/camera/environment/dirty state and renderer binding on ordinary failure | Equivalent refinement. Failed binding recovery propagates to renderer-first teardown. Port 8643 conflict between coexisting apps is reported, not silently ignored. No network handler touches graph/GPU. |
| Integrated multi-target baking | `EditorBaking.cpp` freezes targets/settings/document/renderer context, schedules one live baker and one submitted frame, captures typed/image results after completion and releases each baker before successor | Equivalent serial orchestration. Public bake execution remains independent of app queues. This does not implement StandaloneBakerContext/Batch. |
| Bake adoption/output | All selected-subtree/scene targets finish before preparation/adoption. Warn overlap remains eligible. Unique temporary PNG/RGB8 or HDR archive sources load before material mutation; live/stored linear UV2 baked mode is updated and complete scene save publishes resources | Equivalent refinement plus declared transactional limits. RGB32F export is not clipped, but interactive application rejects values outside finite half range. Preparation removes unused files; setter/tool failure after mutation may leave partial dirty edits and retained sources. M08 must implement its stronger batch guarantees separately. |
| Shutdown/resources | Retire/cancel submitted bake work, wake/stop/join HTTP/CPU readers, detach UI previews/settings, release renderer/extraction before retired scene resources and Engine teardown | Required ownership contract. Per-target previews retain applied images, not a running context. Cancel/resume of import presentation must not revive a discarded bake. |
| Distribution | Shared bundle helper copies engine assets/shaders; recursive app target collector includes both apps and shared runtime/resources | Verification gap: source routing is inspected; native dialogs, install/package launch and shader-only refresh are not established by that inspection. |

Detailed source contracts and limits are preserved in [scene IO](scene-io-cloning.md),
[editing](scene-editing.md), [async import](async-import-service.md),
[scene baking](scene-baking.md) and [product validation](product-validation.md).
Those earlier delivery statuses are chronological; progress is authoritative.

## Production dependency audit and retained scope

The new app uses typed draw resources, not a hidden production renderer. Shared
headers still deliberately retain production includes and overloads: DB
`mesh_obj.hpp`, `mesh_bg2.hpp`, `scene_gltf.hpp`; `DrawableComponent`, Scene and
factory serialization; manipulation selection/gizmos; TextureCache; UserInterface,
UserInterfaceDelegate, TextureWidgets/MaterialEditor; Application and
OffscreenApplication. `scene/all.hpp` still includes `scene/vk/all.hpp`, and the
engine umbrella includes `render/all.hpp`. DB source retains production realization
overloads alongside the new CPU parsed-data entry points. These are real M09
dependencies, not an exemption inferred from a working app window.

Legacy `db::LightmapOutputWriter` still accepts `render::LightmapPixels`; production
standalone context/assembler/batch and `apps/lightmap_generator` remain M08
reference implementations. M07 did not implement a bake output writer, complete
batch filesystem transaction or headless UV scheduler. M08 must deliver these
before deletion. Preserve pure parsers, schema/reference IO, chain joints, subclass
lookup, independent clone/projection contracts and native archive readability.

Native graphics/UI backend code and required SDK assets are not removed by this
review. M09 must distinguish generic ImGui table wrappers from Vulkan backend
implementation; retain CPU UI fixes, GPU Vulkan/Metal, FidelityFX bridge, shared
noise bytes and licensed SMAA lookup data while retiring production-only paths.

## Verification and style record

| Owner/evidence | Actual result |
|---|---|
| Lead continuation | M07-01–06 accepted under the execution protocol; the latest instruction explicitly confirms the selection correction was checked. |
| Agent static review | Inspected public shared scene/clone/parser interfaces, application publication/rollback, job/thread identity, bake completion/adoption, UI table scopes and package-source routing. Checked owned source GPL/style inventory, Markdown paths and diff whitespace. |
| Not specified | Backend/SDK/device, exact HTTP/queue scenarios, baked pixel values, performance, native package/standalone launch and full production regression coverage. No results fabricated. |
| Remaining verification owners | Product matrices in tests/draw/composer_io, composer_import, composer_baking and tests/ui/selectable_list; final native packaging/product checks remain lead-owned through M08/M09. |

Source style inventory covers 43 repository-owned changed/new and Composer
app/shared source files; GPL/adjacent-function scans reported no findings.
The style review covers:
full GPL headers; control-flow braces and new-line block openings at reviewed
sites; unindented namespaces; angled public/necessary private includes; separated
CPP function definitions; purpose-separated method groups in app/public classes.
No source formatting mutation or third-party license change is made by this
review. The necessary private relative httplib include remains app-local.
This scoped review does not certify every legacy source file in the repository.

## Remaining-plan impact and actual edits

| Pending owner | Required implementation / documentation action |
|---|---|
| M07-08 | Document the actual Composer target/UI, root/clone/custom component contracts, Chain and corrected bool table API, async versus stopped realization, reimport/shutdown, serial bake adoption/temporary files and partial-failure limits. Keep production coexistence and unspecified package coverage visible. |
| M08-01 | Implement independent headless scene/job generations and completed samples using public executor/accumulator/composition. Do not copy app frame/import queues or require a camera/RendererDeferred. |
| M08-02 | Reuse root transfer, subclass/clone and CPU parser APIs; exclude helpers, accept camera-less prefabs and prepare independent UV2 replacements before freezing source snapshots. |
| M08-03 | Implement full-batch identities/path collision/protected input/overwrite and actual map/model publication. Composer's per-item export and temporary material adoption are not those guarantees; full float output must not inherit the interactive half-range rejection. |
| M08-05 | Compare real output reload, cancellation/source validity and native deployment separately from the positive selection correction report. |
| M09-01/M09-03 | Inventory remaining transitive production headers/overloads and legacy Composer import/server alongside draw replacements. Preserve neutral parsers/root transfer/custom hooks/helper exclusion and corrected SelectableList API. |
| M09-05/M09-06 | Retain/migrate focused Composer IO/import/bake/table probes and resource roots; validate canonical app, native package and worker/bake teardown after removal. |
| M10-02/M10-04 | Audit final signatures, identity/thread/clone/root/output limits and app/network/distribution guidance against post-M09 source, not historical interim names or generic acceptance claims. |

All three pending milestone summaries and the concrete owning steps named above
are updated with this handoff. M08-04/06/07, remaining M09 and M10 steps were
compared as well: their existing CLI configuration, review/documentation ordering,
native deletion and final-audit responsibilities remain applicable. No step IDs
are reordered and no user architecture choice is changed.

## Exit and next handoff

No unresolved reported selection crash remains after the lead's confirmation.
Explicitly retained limits are not hidden claims of full transactions or package
coverage. M07-07 is delivered awaiting lead review/acceptance. Its successor is
M07-08, not M08. This technical report does not author the cumulative maintained
Composer documentation, promote draw or authorize production deletion.
