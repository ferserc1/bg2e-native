# Drawable instances progress

## Current state

The [drawable scene format](../../doc/api/db/DrawableSceneFormat.md) describes
the current schema. In Composer, edit instance placement through the node
Transform component and its gizmo. The browser shows model-local submesh
matrices read-only. The step entries below record development history.

- Steps 01–10 implemented and accepted. The user confirmed instance lightmaps, then validated owner-based creation from unsaved appearance and explicitly authorized the final documentation step.
- Step 11 documentation delivered; final user documentation review pending.
- Completed implementation/documentation steps: 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11.
- Next step: **None**. No further implementation is planned; documentation is ready for review.
- No agent builds or runtime tests were run for steps 02–11, following the standing user instruction. Step 11 changed Markdown only.

## Step 01 — Common renderable contract

### Changes

- Added `scene::DrawableRenderComponent`, a Component-derived read contract for retained realization, immutable geometry access, effective submesh materials, transforms, visibility and tracing eligibility. No model references, ownership or geometry assignment operations belong to the base.
- `DrawableComponent` and `TextComponent` now derive directly from this base. Text privately owns its generated Drawable; paragraph/material serialization, preparation/recovery and independent cloning remain in Text.
- Kept `Node::drawable()` concrete and added `Node::drawableRender()` for generic consumers. `Node::drawableInstance()` remains for step 03.
- Adapted queue extraction, glyph-mask handling, picking, selection identity, overlays, bounds, camera framing and viewpoint analysis. Generic queue input accepts a const Drawable; legacy mutable selection access is restricted to concrete owner/text components.
- Adapted Composer tree selection, tools, bake occluder snapshots, component membership checks, undo resource accounting/release and complete scene saves. Text retains embedded material packaging and remains ineligible for BG2 model export.
- Audited reflection: Text already has its own registration and does not require the model-specific Swap Drawable action. Compatibility checks now inspect the common renderable base.
- Adapted manipulation/reflection/plugin examples to use validated selection editing access. Bake graph/dependency snapshots retain text as a generic renderable.
- Architecture reference: `architecture.md` is the current available equivalent; `skills.md` is absent. No CMake source, GPU backend or production render namespace changes.

### Build and checks

Existing build tree: `cmake-build-debug`, Debug/Ninja, configured Vulkan SDK `/Users/fernando/VulkanSDK/1.4.313.1/macOS`. The separate `build` cache uses Unix Makefiles and has an empty SDK; it was not used or reconfigured.

- `cmake --build cmake-build-debug -j 4` initially failed because Ninja was not in PATH.
- `PATH="/Applications/CLion.app/Contents/bin/ninja/mac/aarch64:$PATH" cmake --build cmake-build-debug -j 4` built the engine but failed in unrelated GPU example Metal shader compilation: Xcode reports a missing Metal Toolchain. The global build is **not verified**. No toolchain download or generator changes were attempted.
- Targeted builds of `bg2e_composer` and `draw_manipulation tutorial_reflection tutorial_plugins tutorial_text` succeeded after integrating the const contract and its consumers.
- Final combined build: `PATH="/Applications/CLion.app/Contents/bin/ninja/mac/aarch64:$PATH" cmake --build cmake-build-debug --target bg2e_composer draw_manipulation tutorial_reflection tutorial_plugins tutorial_text -j 4` — **passed**, exit code 0 (engine, Composer and all four examples compiled and linked).
- `git diff --check` passes. No new test framework, benchmarks or runtime GPU probes.

### Composer validation

The user reports configuring Metal Toolchain, compiling, and testing Composer with TextComponent and DrawableComponent: selection, duplication, and saving models/text. The user explicitly accepts step 01 as complete. The original checklist below is retained as context; no additional individual checks are claimed beyond the reported results:

1. Open a scene containing ordinary models and Text; verify visibility, glyph transparency, picking, tree selection and selection overlays.
2. Edit text and its material; verify model material editing still works and camera framing includes text bounds.
3. Duplicate Text and a model; verify independent edits, removal and undo/redo.
4. Save/reopen a mixed scene, including non-albedo text material textures; verify paragraph/material preservation and refusal of Text-to-BG2 export.
5. Verify adding Text to an occupied geometry node is refused and offers the existing child-node guidance.

### Recovery limitations and handoff

No new cache or instance functionality is implemented in this step. Existing text recovery boundaries are retained; final bake/offscreen composed-operation work remains step 10. Native dialogs have not been exercised; this existing Ninja tree carries the documented macOS dialog limitation.

User feedback/acceptance: **accepted**. The user completed compilation and the Composer checks described above, and authorized continuing to step 02 without further agent testing. Metal Toolchain availability is user-reported; the earlier global-build failure remains a historical record, not a current blocker.


## Step 02 — Device-backed cache and resource identity

### Implementation

- Added engine-owned `draw::DrawableCache`, created only after a live engine device exists and released before device destruction. The cache has no Scene, UI or DB dependencies. It is synchronous and confined to the serialized resource-update thread, matching the engine's resource mutation policy.
- Added stable `DrawableResourceHandle` identities. BG2 identity/lookup uses normalized absolute paths; procedural identifiers are unique; imported identifiers begin with the source filename and have a unique suffix. Handle copies do not register logical uses. Unload/clear/context teardown invalidate handles; replacement preserves them.
- Kept display labels, original source paths, explicit lookup aliases and export destinations separate. `setExportPath` never adds aliases, so Save As cannot silently change identity or hijack another resource's lookup.
- Added load/find/findIdentifier/registerProcedural/registerImported/replace, enumeration/inspection, usage queries and unload/clear operations. Repeated normalized BG2 loads reuse a fully resident entry without invoking the DB loader again. Reentrant mutations return Busy rather than duplicating partially loaded resources. Publication occurs only after successful realization, topology/device validation and preparation of original definitions.
- Captured original CPU submesh materials via deep copies, plus names, groups, local transforms, visibility and the original Drawable transform. Cache originals remain independent of live edits and Drawable cleanup snapshots. Geometry revisions advance on cache replacement. History snapshots from the same identity can be restored through replacement.
- Added move-only RAII `DrawableUsageLease` registrations for scene owners, scene instances, history, jobs and frames. Logical usage is derived from live registration records, never shared_ptr::use_count or public counter mutation. Owner registration is exclusive; all categories prevent explicit unload/clear. Zero-use entries remain resident until explicitly removed.
- History/job/frame leases pin their acquired Drawable realization and revision, retaining raster buffers and dependent local-space BLAS together across replacement. Cache teardown also releases displaced realizations before device destruction. Frame leases must be retained through GPU completion. Separately extracted native references still follow the engine's existing lifetime contract.
- Added `Drawable::cacheUsage()` backed by weak cache registration. Existing Drawable loading, cloning and geometry mutation signatures/behavior are unchanged. Existing native debug labels remain intact; new resource/usage identifiers require nonempty meaningful labels.
- Added opt-in DB adapter `db::loadCachedDrawableBg2`. Legacy BG2 loaders keep producing independent geometry; automatic scene owner registration and lifecycle integration belong to step 04. Generated Text/overlay geometry is not automatically registered.
- Recoverable mutations return structured, nodiscard diagnostics for invalid inputs, stale/foreign handles, conflicting identities/aliases, active users, incompatible devices and load/resource failures. Failures retain the previous published entry. Unload drains first and releases BLAS before the buffers they borrow.

### Changed areas

New files:

- `lib/include/bg2e/draw/DrawableUsage.hpp`
- `lib/include/bg2e/draw/DrawableCache.hpp`
- `lib/src/bg2e/draw/DrawableCache.cpp`
- `lib/include/bg2e/db/drawable_cache.hpp`
- `lib/src/bg2e/db/drawable_cache.cpp`

Integration: `draw/Engine.hpp` and `.cpp`, `draw/Drawable.hpp`, and the `draw/all.hpp` / `db/all.hpp` umbrella headers. Earlier step 01 worktree changes were preserved. No CMake source or render/GPU backend files changed.

### Build and validation

- **No build command or tests were run for step 02**, following the user's instruction not to perform additional testing. Compilation and runtime behavior for this step are **unverified**; the successful step 01 builds are not evidence for step 02.
- Source review covered API/definition integration, publication failure paths, cache versus device destruction order, stale handles, usage lease lifetime, replacement snapshots and cross-device rejection.
- No new Composer controls exist at this boundary. No additional manual Composer validation is requested in this continuation; step 02 acceptance remains pending user feedback.

### Limitations and handoff

This cache is serialized, not a concurrent/asynchronous loading service. Reentrant in-progress loads return a recoverable Busy result; callers retry after the active operation. Active jobs/frames register on the resource-update thread and keep their lease until completion. Automatic registration of existing scene owners, scene New/Open behavior, command history and offscreen jobs remain the integrations explicitly scheduled for step 04 and later steps.

Cache identities and geometry revisions apply to registered resources and replacements; existing scene geometry edits are not rerouted in this step. No cache persistence, unused-entry serialization or UI browser has been introduced.

User feedback/acceptance for step 02: pending. Exact next step: [03 — Instance component and rendering](03-instance-component.md). Stop here; do not start step 03 without a new continuation request. Do not begin final documentation until every implementation step has user acceptance.


## Step 03 — Instance state and rendering

### Implementation

- Added `scene::DrawableInstanceComponent` directly under `DrawableRenderComponent`, with factory/reflection registration and the concrete `Node::drawableInstance()` accessor. `Node::drawable()` retains its owner-only meaning.
- Instance binding acquires a stable cache handle and a lifetime-safe instance-use registration. It copies original CPU submesh materials, local transforms, names/groups, visibility, global Drawable transform and tracing eligibility. Reassigning the same live handle is idempotent; invalid or cross-engine assignments retain the previous binding.
- Instance state contains no mesh or Drawable clone. The public geometry view is `shared_ptr<const Mesh>`; there are no geometry mutation setters. The legacy Drawable bridge returns null for instances, including via a base cast, because const Drawable exposes mutable mesh storage. Renderer-only access to the original realization uses a protected bridge with explicit friends.
- Added independent authored materials and independently realized MaterialBase objects. Material changes are realized at stopped resource boundaries, preserving prior complete GPU snapshots through frame ownership. Failures warn, retain authored data, suppress only the affected instance and wait for a changed material/source or explicit retry rather than retrying every frame.
- Cloning shares the stable geometry identity and raster/BLAS realization, while deep-copying authored material/texture settings, transforms and metadata and realizing independent materials. It never builds an instance-specific raster mesh or BLAS.
- Geometry revisions trigger index-based submesh reconciliation before rendering: excess state is truncated and missing state appended from original definitions, with a warning and no cancellation of higher-level operations. Existing indices retain their authored materials/transforms/metadata.
- Added a RenderQueue component overload, retaining the legacy Drawable overload. Extraction uses each component's effective material, submesh visibility, transform and tracing eligibility, with the original shared local-space raster buffers/BLAS. Existing TLAS extraction already consumes the retained queue item, so it receives the effective world matrix and material flags without GPU API changes.
- Queue items hold frame-use leases as well as their raster/BLAS/texture snapshots. Copies retained by tracing collections preserve those lifetimes. Frame leases remain alive through queue/collection ownership until GPU completion, and prevent explicit cache unload while retained.
- Extended the common read contract with geometry availability/identity, engine and metadata accessors. Bounds, camera framing, selectable identifiers, picking, selection restore, existing Composer submesh lists and selection commands now use that contract rather than requiring a mutable Drawable.
- Added `SelectionItem::forRenderable` and an opaque resource identity. Picking/selection works for instances, while `lockMesh()` intentionally returns no editable geometry for them. Existing owner/text editing remains available. Metadata deduplication identifies the component rather than conflating instances sharing a source.
- Overlay preparation accesses shared geometry internally and reuses edge meshes per raster realization. Overlay rendering already uses the queue's effective material/world matrix. Viewpoint analysis already follows the queue and common component identity.
- Instance resource preparation runs from existing default delegate and deferred-renderer preparation boundaries, including the inherited offscreen delegate path. Traversal/extraction performs no GPU realization or geometry repair.
- Added a cache lease overload with a weak-payload device-release hook. Cache shutdown clears independent instance material realizations before destroying the engine device, retaining CPU authored state. Reentrant mutations during shutdown callbacks are refused. Added a nonallocating handle geometry-revision query for resource observation.
- Adapted generic bake dependency/snapshot traversal to effective component state, and Composer occluder cloning to clone instance appearance while sharing its geometry. Full instance bake target/output integration remains step 10.

### Changed areas

New files:

- `lib/include/bg2e/scene/DrawableInstanceComponent.hpp`
- `lib/src/bg2e/scene/DrawableInstanceComponent.cpp`
- `lib/src/bg2e/scene/reflection/DrawableInstanceComponentReflection.cpp`

Integration: common renderable contract, Node/scene umbrella, DrawableCache handles/leases, RenderQueue/visitor, preparation boundaries, bounds/camera framing, selection/picking/overlays, generic bake traversal, Composer selection/list/occluder adapters. Existing step 01/02 changes were preserved. No CMake source, shaders, bg2e::render or GPU backend files changed.

### Build and validation

- **No compilation or runtime tests were run for step 03**, respecting the standing instruction not to run additional builds/tests. This step is **unverified** by compilation or execution; earlier successful builds do not validate these changes.
- Source review covered declarations/definitions and consumers, read-only instance geometry access, independent material state, replacement topology, stale cache handles, frame/BLAS retention, cache/device shutdown order, and recovery boundaries.
- `git diff --check` passes. No build system source files changed.
- User acceptance and Composer validation for this step remain pending. The plan's existing ordinary geometry/text visibility and selection checks remain applicable; no new dedicated instance controls exist yet. No interactive checks are claimed.

### Recovery limitations and handoff

Instance persistence belongs to step 05. Current instance serialization explicitly refuses to produce a lossy representation; Node propagates that refusal instead of silently dropping the component. The registered instance factory can create an unbound CPU component; scene-format loading warns that binding/state restoration is not available at this boundary.

Scene owner registration/exclusivity, command retention/memory accounting, New/Open residency and context publication remain step 04. Composer's resource browser and dedicated instance editing/gizmo remain steps 07/08. Generic picking and selection are ready, but legacy mutable geometry/material editing paths must not be used for instances. Full bake/output recovery remains step 10.

Direct renderer users must prepare resources at a stopped boundary after material/source changes and retain extracted queues/collections through GPU completion. Separately extracted native handles retain the engine's existing lifetime requirements. Failed preparation/extraction suppresses the affected instance until a source/material change or explicit retry; authored CPU state remains available.

User feedback/acceptance for step 03: pending. Exact next step: [04 — Ownership and lifecycle](04-ownership-lifecycle.md). Stop at this boundary; do not begin step 04 without a continuation request. Final documentation still requires acceptance of every implementation step.


## Step 04 — Exclusive ownership and scene lifecycle

### Prerequisite confirmation

The user reported that compilation is correct and requested the next step. This confirms compilation of the integrated steps 02/03 and accepts their continuation boundary. Their historical “unverified” notes above describe the agent checks at those boundaries; no additional Composer runtime checks are inferred.

### Implementation

- Authored DrawableComponent resources reserve exclusive ownership immediately, including CPU-only construction. Registration with a device-backed cache occurs at resource preparation, covering loaded, procedural and imported geometry. Text and tool overlays remain outside authored owner registration. Node compatibility restrictions are unchanged.
- Constructors, assignment, rebinding, cloning and load paths enforce one owner. Invalid assignments preserve the prior binding and return structured diagnostics without opening UI. C++ owner copies and component clones produce independent geometry and identities. Raw-pointer construction refuses an already owned resource before creating a second owning shared_ptr.
- Owner setDrawable replaces the existing stable entry; explicit rebindResource changes only that component's identity. Geometry edits publish observed revisions at stopped preparation boundaries, allowing instances to reconcile by index while retaining authored appearance. Removing an owner leaves resident geometry, original definitions and instances available.
- Clones retain actual import provenance separately from their distinct planned BG2 destinations. Reference-only serialization refuses unpublished clone destinations. Physical duplicate publication and scene-format migration remain step 05; coordinated export remains step 06.
- Added engine-tracked cache scopes, inherited node loading contexts and lease-held context lifetime. True New/Open prepares a fresh candidate scope; failed opening retains the existing document. Existing bindings retain their original scope when nodes move. Engine teardown invalidates all scopes and releases current and displaced native resources before the device.
- Internal cleanup reloads retain the previous runtime cache scopes on the candidate document. This provides the residency anchor for step 09; disk dependency protection and cleanup policy are not completed here. Zero-use entries are not automatically evicted.
- Added explicit history/job retention helpers. Composer command history and clipboard retention register resource uses; instance memory accounting includes independent material state without charging shared geometry repeatedly. Retired construction references are pruned without destroying resident cache entries.
- Integrated resource-use pins with Composer lightmap jobs and integrated/standalone baker contexts. Pins survive until native work and retained queue state are released. Full instance bake output/recovery remains step 10.
- Preserved actual BG2/OBJ/glTF source provenance. Repeated glTF mesh nodes now receive independent owner clones rather than attempting to attach a second owner to one Drawable.
- Composer model export snapshots the existing owner directly through a model-only DB helper, avoiding a transient second owner. Legacy strict model export validation remains. Existing setters and cleanup consumers now handle assignment diagnostics and cache residency explicitly.
- Cached Drawable cleanup warns and preserves residency; explicit cache unload/clear owns native release. Uncached cleanup retains its existing behavior. Cache shutdown preserves CPU material state and releases BLAS before dependent raster buffers.

### Changed areas

Ownership and loading: Drawable, DrawableComponent, DrawableCache/Usage, Engine, Node, DB cache adapter and BG2/OBJ/glTF loaders. New scene retention helpers: `lib/include/bg2e/scene/drawable_resources.hpp` and `lib/src/bg2e/scene/drawable_resources.cpp`.

Consumers: Composer StageScene/DocumentIo, command retention, bake jobs, model-only export, model_edit cleanup, UV2 reload, gizmo assignment and lightmap example. Earlier step 01–03 changes were preserved. No CMake files, shaders, production bg2e::render implementation or GPU backend internals changed.

### Build and validation

- **No compilation or runtime tests were run for step 04**, following the standing user instruction. The user's successful build confirms the preceding step, not this implementation.
- Source review covered exclusive reservations, failed-assignment preservation, replacement versus rebinding, independent copies, import provenance, context teardown, history/frame/job retention and candidate document publication.
- `git diff --check` passes.
- User compilation/Composer validation remains pending. Applicable checks are ordinary model load/import/duplicate, replacement, remove/undo/redo, clipboard, New/Open and failed Open. No new instance controls are exposed yet; no interactive checks are claimed.

### Limitations and handoff

Instance persistence and physical legacy duplicate files remain step 05; shared export coordination remains step 06. Browser/editing controls, disk cleanup protection and complete instance bake/output recovery remain steps 07–10. Native runtime residency and disk dependency analysis are separate responsibilities.

User acceptance for step 04: pending. Exact next step: [05 — Scene format and legacy duplicates](05-scene-format.md). Stop at this boundary until the next continuation request.

### Follow-up compilation correction

The user's step 04 build reported a declaration/definition mismatch for StageScene::load. Restored its existing three-argument declaration and original defaults; the cache parameter belongs only to the private preparation helpers. The definition and both DocumentIo call forms now match that declaration. No agent compilation or runtime tests were run; user build confirmation remains pending.


## Step 05 — Instance scene format and legacy duplicate recovery

### Prerequisite confirmation

The user explicitly requested marking step 04 complete and continuing after the StageScene::load declaration fix. Step 04 is accepted; no additional compilation or individual runtime results are inferred from that instruction.

### Implementation

- DrawableInstance JSON now contains a model `path`, global `transform`, authored `rayTracingEnabled`, and an ordered `submeshes` array. Each item stores independent `material`, local `transform`, `name`, `groupName` and `visible` state. Material JSON includes all six image slots, sampler settings, UV/scalar controls, indirect mode and lightmaps. Explicit null image slots preserve removal of original textures.
- Graphics-free deserialization retains an independent representation and normalized resource reference without creating a cache or device. Runtime binding loads through the node's existing device-backed context; same normalized paths reuse one entry. Defaults come from immutable resource originals before the serialized overlay is applied by index.
- Surplus items are ignored, missing items/fields retain resource originals, and malformed optional values warn while retaining recovered/default values. Validation covers matrices, material field types, sampler enums, UV/channel indices and indirect texture interpretation. Instance-specific parsing does not weaken existing direct material parsing contracts.
- Missing geometry and failed material realization preserve reference and authored appearance. Deferred overlays survive clone/undo retention, failed loads and serialization to another base directory; unresolved model and material references are rebased. Explicit retry enables another failed load attempt without per-frame warning loops. Mutable unresolved material edits and direct property setters remain represented in the deferred overlay.
- Legacy duplicate-owner resolution retains the first normalized-path resource. Later owners keep independent Drawables/identities and receive unused BG2 destinations. Diagnostics explicitly state that ambiguous same-path instances remain attached to the first resource; no material/name-based inference is performed.
- Independent owner destinations are checked against filesystem entries and planned cache paths. BG2 byte copies are attempted without overwrite at resource registration, in the source directory so relative image references retain their base. Successful copies add only their new lookup alias; no duplicate receives the original path's alias. Copy/permission failures warn and retain independent memory plus a pending export destination without cancelling scene loading. Other import formats retain pending destinations for proper BG2 export.
- Extended graphics-free scene reference/dependency collection to instance model paths and all submesh material images, including lightmaps. Texture progress counting includes embedded instance materials.
- Complete scene saving now explicitly reports a partial-save warning when it encounters instances: their reference JSON is preserved, but coordinating newly exported owner paths, ownerless resources and image packaging belongs to step 06. Reference-only IO is available at this boundary; complete shared-resource saving is not reported as fully successful prematurely.

### Changed areas

DrawableInstanceComponent header/implementation, DrawableComponent independent-copy publication, DB scene dependency/texture traversal, and the existing complete-save diagnostic boundary. Earlier worktree changes were preserved. No CMake files, shaders or graphics backend APIs changed.

### Build and validation

- **No compilation or runtime tests were run**, following the user's standing instruction.
- Source review covered declaration/definition matching, JSON field types, default-overlay order, explicit null textures, path rebasing, deferred state/clone recovery, stable cache lookup, first-owner resolution, non-overwriting copies and partial-save reporting.
- `git diff --check` passes.
- User compilation/Composer validation is pending. Relevant checks are legacy duplicate scene opening, independent duplicate model paths, failed copy recovery, and reference-only instance JSON round trips with fewer/more submeshes and unavailable model/image files. No dedicated instance controls exist yet and no manual checks are claimed.

### Limitations and handoff

Complete shared model/image export, intended-path overwrite policy and coordinated Save As publication remain step 06. This step's clone-copy operation never overwrites an unrelated destination. Browser/editing controls remain steps 07/08, disk-residency cleanup protection step 09 and full bake/output recovery step 10.

User acceptance for step 05: pending. Exact next step: [06 — Shared resource export coordinator](06-save-coordinator.md). Stop at this boundary until the next continuation request.


## Step 06 — Shared resource export coordinator

### Prerequisite confirmation

The user reported successful compilation of step 05 and requested the next implementation step. Step 05 is accepted for continuation. No additional individual Composer runtime checks are inferred.

### Implementation

- Replaced per-node model snapshots with an operation-local ResourceExports table. Only identities referenced by serialized owners/instances enter the table; unused resident cache entries are not exported. Explicit job leases retain the referenced realizations throughout the operation.
- Each shared identity is written once. An owner in the exported graph supplies its current material, visibility, name/group and transform properties and node joints. Without an exported owner, the table uses immutable original submesh definitions and original global transform; instance appearance never alters the model defaults.
- Instance serialization emits only JSON through an operation-local export view. All six image slots, sampler/UV/scalar controls, indirect/lightmap state, metadata and transforms remain independent. No instance-specific BG2 writing, geometry copy or mutation of cached original materials is introduced.
- BG2 snapshots retain the existing transform-baking behavior. Instance local matrices are compensated by the inverse exported submesh basis, preserving their effective geometry placement on reload. Successful publication records that basis separately from immutable originals and lookup aliases; raw reference serialization and binding an exported reference to an already resident resource use the same conversion. CPU instance publication references/bases survive ordinary cloning and resource recovery.
- Owner representation snapshots preserve virtual custom serializers while restoring authored/pending paths on every exit. Model and instance serialization do not publish planned references prematurely. Runtime owner destinations, instance reference bookkeeping and cache publication metadata are updated only after disk commit.
- Normal saves reuse intended model filenames in the destination directory and replace those files, so subsequent edits persist rather than creating a new BG2 for every save. Save As consolidates referenced resources in the destination directory. Existing unrelated files, duplicate-resource filename collisions and unpublished collision-copy destinations receive unused names; source lookup aliases are not retargeted by Save As.
- Material/environment/font/notice file identity uses normalized full source paths, preventing equal basenames from silently sharing unrelated images. Source images are copied from the authored file, rather than preferring potentially stale files already in the destination. Procedural material pixels are exported from independent CPU snapshots or a validated current native image.
- Publication stages all resources, installs image dependencies before BG2 models, and atomically replaces the scene document last. Intended resource replacements retain rollback copies until the document succeeds. Failed publication removes newly installed files and restores replaced files. If the OS also refuses rollback, its backup directory is retained and reported rather than silently discarded.
- Resource failures are isolated and aggregated while independent entries continue staging. A failed model may retain a complete existing BG2 and its existing images with a partial-save warning. An instance image failure preserves authored references where existing targets are available. If no safe model/image target exists, the operation returns controlled failure before publication, preserving the prior document/files and live edits. Existing Composer reporting keeps partial saves dirty and does not authorize document replacement.
- Removed the temporary step 05 “shared model export coordination pending” warning. Reference-only saving and graphics-free raw document IO remain separate from complete export. No top-level drawables array or unused-residency persistence is emitted.

### Changed areas

`db/draw_resource_io.cpp` contains the operation-local table, common snapshot/material packaging and transactional publication. `db/scene.hpp` documents the complete-save result contract. DrawableComponent adds an export representation helper and pending-destination query. DrawableInstanceComponent adds an independent export representation/publication view and basis-aware reference restoration. DrawableCache stores published export paths/bases separately from source lookup aliases and original definitions.

Earlier worktree changes were preserved. No CMake files, shaders, bg2e::render implementation or GPU backend APIs changed.

### Build and validation

- **No compilation or runtime tests were run**, following the standing user instruction. The user's successful build validates step 05, not this implementation.
- Source review covered declarations/definitions and consumers, resource identity grouping, ownerless defaults, texture identity, independent instance material JSON, transform-basis conversion, normal overwrite versus collision copies, metadata publication order, resource/file rollback and partial-save reporting.
- `git diff --check` passes.
- User compilation/Composer validation is pending. Applicable checks are save/reopen after owner edits; repeated save to the same model path; Save As with basename collisions; shared and ownerless instance round trips; independent instance textures/lightmaps/transforms; unavailable images and unwritable destinations. No new dedicated UI is exposed yet and no manual checks are claimed.

### Limitations and handoff

The transaction protects engine-reported publication failures; crash/power-loss recovery and concurrent external edits are not a cross-file journal. The engine's stopped resource-update boundary remains required. If external permission/media changes prevent rollback, the reported staging backups require recovery. Browser controls, instance editing/gizmo, disk residency protection and full bake/output recovery remain steps 07–10.

User acceptance for step 06: pending. Exact next step: [07 — Composer browser and instance creation](07-composer-browser.md). Stop at this boundary until the next continuation request.


## Step 07 — Composer cache browser and basic instance creation

### Prerequisite confirmation

The user reported that step 06 compiles and requested the next step. Step 06 is accepted for continuation; no additional individual runtime checks are inferred.

### Implementation

- Added `ui::DrawableInstancesWindow`, available from **Window > Drawable Instances** in Composer. The engine UI wrapper owns every ImGui call; public headers and application consumers expose no ImGui types or functions.
- The browser lists each resident identity, label, source/export paths, cache scope and geometry revision, plus owner, instance, history, job and frame registration counts. It includes the active document scope and additional scopes retained by the document or its bound components, without conflating equal paths from distinct scopes.
- Read-only snapshots are captured at Composer's frame/resource boundary. The UI retains only handles and CPU display values, never a geometry realization, owning cache reference or usage lease. Opening the browser therefore does not itself prevent unloading. Snapshot selection resets across document generations and follows newly loaded entries.
- **Load BG2 into cache** and **Unload resource** use the existing pending document-IO path, with device draining, document-generation checks and fresh cache diagnostics. Repeated normalized loads reuse the resident entry. These operations neither submit undo commands nor mark the scene modified. Unload is disabled with an explanation whenever any logical use prevents it and is revalidated at execution.
- **Create instance** inserts a node under the selected parent (or the document root), binding independent instance appearance to the selected shared resource. **Create instance from selected owner** is available in both the browser and **Create Node > Instance from selected owner**; it creates a sibling at the owner's node pose, or a child when the owner is the document root. Initial appearance uses the resource originals, following the agreed instance contract.
- Creation and instance deletion use existing reversible node commands. Changing a selected instance's resource uses a prepared independent replacement plus the reversible component-membership command, preserving node placement and retaining exact before/after instance states for undo/redo. The button explicitly states that the new resource resets appearance to its originals. No editable owner or mesh clone is created.
- Detached preparation and command execution use stopped renderer boundaries, generation validation, command-budget admission and existing retained-resource retirement. Direct binding errors, rejected commands and cache IO failures produce message boxes; composed command diagnostics also reach the existing status/history infrastructure. Invalid preparation leaves the existing scene component untouched.
- Added read-only inspection of the selected owner/instance's resource identity, drawable/local submesh matrices, names/groups, visibility, material scalar/color/flags, texture UV indices, images and indirect/lightmap source. Full per-instance editing and the dedicated gizmo remain step 08. Generic unbound instance addition now points to the working browser entry point.

### Changed areas

New files:

- `lib/include/bg2e/ui/DrawableInstancesWindow.hpp`
- `lib/src/bg2e/ui/DrawableInstancesWindow.cpp`
- `apps/bg2e_composer/src/DrawableInstances.cpp`
- `apps/bg2e_composer/src/DrawableInstancesUi.cpp`

Integration: UI umbrella header, Composer AppDelegate/EditorUi declarations and menus, frame snapshot refresh, DocumentIo cache operations and instance-add guidance. Existing command/retention implementations are reused. Earlier worktree changes were preserved. No CMake files, shaders, production rendering implementation or GPU backend APIs changed.

### Build and validation

- **No compilation or runtime tests were run**, following the standing user instruction. The user's successful build confirms step 06, not these changes.
- Source review covered declarations/definitions, UI-only ImGui use, snapshot lifetime/const access, stale document/handle checks, independent detached preparation, existing command/undo retention, repeated cache load, unload guards and diagnostics.
- `git diff --check` passes.
- User compilation/Composer validation is pending. In **Window > Drawable Instances**: load a BG2 twice and verify one resident identity; create/select instances; inspect their material/matrix data; save/reopen; undo/redo create, delete and resource changes; verify unloading is refused while scene/history/jobs/frames retain the entry. An unused explicitly loaded resource should unload without changing dirty state or history.

### Limitations and handoff

This step exposes inspection and basic binding actions. Independent appearance editing, owner replacement UI and the small hidden-by-default instance gizmo remain step 08. Disk dependency protection for cache-only resources remains step 09; runtime unload guards are separate from disk cleanup. Complete instance bake/output recovery remains step 10. No runtime interaction is claimed as verified.

User acceptance for step 07: pending. Exact next step: [08 — Composer editing and instance gizmo](08-composer-editing-gizmo.md). Stop at this boundary until the next continuation request.

### Follow-up: instance node transform gizmos

The user reported that instance node transforms could be edited in the component inspector but not with transform gizmos. Composer's exact-type transform-writer allowlist omitted DrawableInstanceComponent, so the Begin observer rejected the interaction before any matrix update. Added that concrete type to the existing authored-node transform transaction path, preserving the restrictions for unknown/custom writers. Translation, rotation and scale continue through the same preview, cancellation and undo/redo transaction as ordinary nodes. No compilation or runtime tests were run; user verification remains pending.


## Step 08 — Instance editing, owner replacement and gizmo

### Prerequisite confirmation

The user explicitly accepted step 07 and requested continuation after reporting the transform-gizmo issue, receiving its fix, and confirming that submesh material editing was scheduled for step 08. No additional unreported build or runtime results are inferred.

### Implementation

- Extended SelectionItem with identity-validated appearance locks for the renderable, native material and authored material definition. Legacy lockMesh remains unavailable for instances, preserving read-only geometry.
- Composer's existing submesh properties and material editor now accept instance selections. Independent names, groups, visibility, drawable transforms and local submesh transforms are editable. Matrix controls use validated affine, finite, invertible values and the existing draft/commit editing session; invalid values retain the prior scene state and produce diagnostics.
- Adapted continuous material value transactions, discrete texture/sampling/indirect-map changes, copy/paste and material JSON patches to each component's authored definition and independent native material. Undo/redo retains material state and image tickets, without charging shared instance raster buffers or BLAS. The non-Composer queued material editor also publishes independent instance appearance through a private material snapshot.
- Added nonallocating material revision acknowledgements to instance state. Successful edits update both CPU and native appearance without causing resource preparation to replace native material identities behind active/history commands. Geometry-only revisions with unchanged appearance preserve those identities as well. Direct authored source changes still trigger normal preparation/recovery.
- Geometry controls are hidden for instances and disabled in the Edit menu when a selection includes an instance. Direct/mixed selection geometry actions reject the operation rather than silently applying it to owners only. Existing geometry/UV2 command adapters retain their mutable-owner lock checks. Text geometry/source protections remain separate.
- Added reversible owner replacement through **Replace owner geometry...** and the existing **Swap Drawable** component action. A prepared independent BG2 replaces the owner's stable cache entry, so instances follow the new geometry. Source/export identity remains stable; ordinary assignment conflicts keep the prior owner and report their diagnostic.
- Replacement prepares detached before/after instance appearance states against the candidate geometry, without copying its raster mesh or BLAS. Existing indices retain authored properties, excess state is truncated and new indices use candidate defaults with a warning. Undo/redo restores exact component/material identities and topology-dependent state. A replacement requiring unavailable instance materials is rejected locally with a diagnostic until those materials are resolved.
- History retains both geometry realizations once, independent instance appearance through existing SceneRetention, and explicit History leases. Captured original submesh defaults/global transforms can be restored only from the same identity's History version after its exact realization is restored; undo therefore also preserves the correct defaults for future/ownerless instances. Replacement publication and compensation restore selection/membership alongside geometry, with controlled history fault reporting if compensation itself fails.
- Replaced the standalone reflected Swap Drawable placeholder with a file picker and stopped scene update. Direct failures display message boxes. Text and overlay helpers do not gain this owner action through inheritance.
- Added GizmoType::DrawableInstance, appended without renumbering existing types. Its small screen-size-aware shape consists of two offset cube cages built from thin edge bars. It is hidden by default, has independent visibility/scale/opacity controls in **UI Settings > Gizmos > Drawable Instance**, and reuses the existing overlay/picking pipelines. Generated helpers remain outside model ownership/cache, final rendering and bake; the transform gizmo remains a separate facet and uses the corrected authored-node transaction path.

### Changed areas

SelectionManager appearance helpers, DrawableInstanceComponent preparation/revision support, cache History defaults snapshots, DrawMaterialEditor, Composer material and metadata commands, submesh tools, geometry menu guards, owner replacement UI/reflection, GizmoComponent and UISettingsWindow.

New files:

- `apps/bg2e_composer/src/command/material/MaterialTargets.hpp`
- `apps/bg2e_composer/src/command/scene/ReplaceDrawable.hpp`
- `apps/bg2e_composer/src/command/scene/ReplaceDrawable.cpp`

Earlier worktree changes were preserved. No CMake files, shaders, bg2e::render implementation or GPU backend APIs changed.

### Build and validation

- **No compilation or runtime tests were run**, following the standing user instruction.
- Source review covered appearance versus geometry access, material editor target capture and source policies, native material identity retention, matrix/metadata undo, mixed geometry selection guards, stable owner replacement, detached instance snapshots, original-default restoration, compensation, memory tickets and helper exclusion.
- `git diff --check` passes.
- User compilation/Composer validation remains pending. Applicable checks: select instance submeshes and edit colors/textures/sampling/indirect maps, matrices and visibility; verify the owner and other instances remain unchanged; duplicate/delete/undo/redo; replace an owner with fewer/more submeshes and undo/redo back through earlier material edits; save/reopen; verify Text material/source behavior; toggle the instance type gizmo and confirm node transform handles remain operational.

### Limitations and handoff

Resident-resource disk cleanup protection remains step 09. Complete instance bake/output and offscreen composed-operation recovery remain step 10. A direct standalone replacement uses the host's stopped scene-update loop; applications implementing their own loop continue to use the validated assignment API at their explicit resource boundary. No runtime result is claimed as verified.

User acceptance for step 08: pending. Exact next step: [09 — Resident resources and disk cleanup](09-cleanup.md). Stop at this boundary until the next continuation request.

### Follow-up: staged BG2 material image paths

The user reported a partial-save error after loading a BG2 into the cache, creating two instances and changing one instance material. Complete export passed final publication paths to storeMeshBg2, whose existing image-copy phase attempted to read those files before commit. Added an operation-local material path policy: BG2 snapshots reference existing staging images, while instance JSON keeps final published paths. This applies to owned/ownerless shared models and standalone model snapshots without changing storeMeshBg2's public contract or live authored texture paths. No compilation or runtime tests were run; user save/reopen verification remains pending.

### Follow-up: saved scene immediately marked modified

The user confirmed successful saving but reported “Unsaved changes — Saved …” in Composer's status bar, without a partial-save warning. Successful saves already call markSaved; the camera input path also forwarded shortcut keys to orbit navigation. Ctrl/Cmd+S could therefore enter fly mode, with a queued key-up or ongoing camera motion producing a nonhistorical change after the save. Composer now excludes Ctrl/Super key-downs from navigation using modifier state captured before deferral; key-up handling remains intact. Scene saving settles existing orbit input and its smoothing tail at the visible pose before serialization and markSaved. Partial-save warnings and history dirty tracking remain active. No compilation or runtime tests were run; user verification remains pending.


## Step 09 — Resident resources and disk cleanup

### Prerequisite confirmation

The user confirmed successful checks after the step 08 save and dirty-state corrections, explicitly accepted that step, and requested continuation. Step 08 is complete based on that user acceptance; no additional agent build/runtime result is inferred.

### Implementation

- Added transient file/model roots and stable cache identity/generation values to resource analysis, review and execution. Existing file-only overloads remain available without a scene, engine or device. Omitting runtime protection cannot execute a runtime-reviewed plan.
- Resident cache dependencies include cache-only entries, source aliases, published models, immutable originals, current owner materials, retained geometry versions and explicit usage payloads. Independent instance state supplies weak dependency readers; material history retains the union of before/after texture sources. Owner and instance frame leases capture appearance paths independently of later edits. Generic history/job/frame retention aggregates all component appearances before identity deduplication, so multiple instances of the same geometry retain every required source. All six filesystem texture slots are included, including the indirect/lightmap slot; procedural textures do not invent disk references.
- Added content generations for resource load (including an already resident request), registration, unload/clear, replacement/default refresh and export/alias publication. Ordinary lease/frame turnover does not advance generation. Composer invalidates resource review on nonhistorical browser load/unload and checks reviewed cache generations before continuing cleanup.
- Extended CPU dependency traversal to BG2/VWGLB textures, OBJ/MTL material sources, glTF/GLB buffer/image URIs and nested scene model references, with visited-model cycle protection. Existing missing/unsupported dependency warnings, explicit warning acknowledgement, disk fingerprints, symlink exclusions and per-file removal checks remain active.
- Shared document context enumeration between the browser and cleanup. The internal cleanup reload explicitly transfers every reviewed document context, including nested/bound scopes, to the candidate root. Cache-only residency survives; true New/Open still creates a fresh scope without those anchors. No runtime roots are persisted.
- Cleanup rebind verifies every reviewed cache generation and the complete filesystem snapshot. It carries the reviewed roots across history/clipboard retirement, permits added protection only when the deletion list shrinks, and rejects changed/new files or newly discovered warnings. Execution reanalyzes the rebound snapshot before removal. The existing delete confirmation and transactional document adoption remain in place.
- Audited DrawableRegistry: its implementation manages unique drawable display names and a separate strong list; repository searches found no active registerDrawable callers. It is not used as a resident-cache or disk-inventory authority.
- Updated the resource window explanation and confirmation to describe protection and preserved residency.

### Build and validation

- **No compilation or runtime tests were run**, following the standing user instruction. Step 09 compilation is unverified.
- Reviewed declarations/definitions, all cache mutations, lease lifetimes, history material sources, dependency closure, stale review rejection, internal reload ownership transfer and true New/Open isolation.
- `git diff --check` passes. Earlier worktree changes were preserved; no CMake, GPU backend or rendering pipeline changes were made.
- User validation pending: load an otherwise unused local BG2 into the cache, analyze and verify its model/textures are excluded from deletion; load another model after analysis and verify a new analysis is required; run cleanup and verify cache entries still exist and can create instances; truly reopen the saved scene and verify unused nonpersisted entries no longer protect files. Also verify independent instance texture/lightmap paths, material undo/redo sources, warning acknowledgement and external file-change rejection.

### Limitations and handoff

Protection is conservative for the reviewed operation: sources required by the retired history/clipboard remain protected for that cleanup. A subsequent analysis may release those roots once no resident or retained resource needs them. If reload changes disk contents or introduces previously unseen external dependencies/warnings, cleanup stops before deletion and requires a fresh review. Runtime behavior remains unverified by the agent.

User acceptance for step 09: pending. Exact next step: [10 — Bake, offscreen and composed-operation recovery](10-bake-offscreen.md). Stop at this boundary until the next continuation request.


## Step 10 — Bake, offscreen and composed-operation recovery

### Prerequisite confirmation

The user confirmed that cache and scene cleanup work correctly, explicitly marked step 09 complete and requested the next step. Step 09 is complete based on that acceptance; no additional agent verification is inferred.

### Implementation

- Added receiver overloads to UV surface preparation and the bake executor. Integrated and standalone contexts resolve a generic receiver separately from its retained shared Drawable/raster buffers. Each instance supplies its own combined drawable/submesh transforms. Prepared geometry retains a Job lease and the receiver; stale resource/component/native geometry replacement is rejected. Existing direct-Drawable entry points and owner surface-pass hooks remain available.
- Composer selected-subtree, entire-scene and individual-target bake now include loaded instances. Owner bake snapshots use independent instance appearance bound to the existing resource, avoiding raster/BLAS clones for registered model owners. Existing instance snapshots retain their independent appearance. Text remains an occluder, with its existing procedural UV protections; it is not added as a model bake receiver.
- Captured destination component/resource identity and geometry version for each job. Applying completed output validates membership and geometry before preparing any receiver changes. Per-target images are attached through the existing PreparedMaterial/EditMaterials command, using generic SelectionItem appearance locks and authored definitions. This preserves independent instance lightmaps, generated source ownership, undo/redo and normal scene material packaging on save/reopen. Loaded baked bindings can be previewed on instances.
- The UV/lightmap window selects and previews instances, including uploaded-geometry replacement tracking. Generate UV2 is disabled for instance receivers with an explanation; the direct Composer action reports the same owner-only requirement. Existing UV2 on shared geometry is used independently by every receiver.
- Standalone prefab assembly resolves device-backed instances, collects them as separate hierarchy-based output identities and validates their materials, transforms and existing UV2. Instance identities/appearance are not replaced with owned geometry. CPU-only target loading or atlas regeneration for instance receivers fails with an explicit diagnostic; generate UV2 on the shared owner first.
- LightmapOutputWriter accepts instance targets for image/metadata/manifest output, protects instance model paths and resident/retained dependencies, and holds Job resource leases. Its scene signature includes document cache identities/generations. Existing filesystem fingerprints, staged generations, destination checks and manifest-last publication remain strict. Optional standalone BG2 model copies remain owner-only: requesting copies for instance targets is rejected during preflight rather than exporting or changing their shared model.
- Added `--cached-model FILE` to the existing `draw_offscreen_application` example. It loads the same BG2 twice before scene-node binding, requires one cache identity, retains a Job use and renders two independent material instances. This is an opt-in existing-surface exercise with an initialized backend, without Composer, an HTTP server, batching or a new test framework. It was not executed.
- Composer incremental Open continues loading other scene geometry after recoverable model-resource failures. It preserves unresolved components/references and reports accumulated resource warnings once after successful transactional adoption. Duplicate owner destinations and incomplete physical copies are included. Internal cleanup reload rejects those warnings before adoption/deletion.
- Added existing component dependency-issue overrides for owner assignment/load failures, pending independent model copies and unresolved instance resources. In-memory state and pending destinations remain explicit; incomplete writes are not represented as successful saves.
- Successful New/Open retires the previous document's bake jobs and queued application eligibility at the stopped boundary, releasing their cache/resource retention. Failed candidate loading keeps the prior document and its jobs. Existing shutdown cleanup continues draining submissions before resource release.

### Changed areas

UV surface/executor declarations and implementations, integrated/standalone bake contexts, standalone assembly, LightmapOutputWriter, Composer bake jobs/material application and UV window, incremental document IO, drawable dependency diagnostics and the offscreen application example. Added the private engine implementation header `lib/src/bg2e/bake/Receiver.hpp`.

### Build and validation

- **No compilation or runtime tests were run**, following the standing user instruction. Step 10 compilation and graphics behavior remain unverified by the agent.
- Source review covered receiver/resource identity, independent transforms/materials, shared raster/BLAS retention, output identity, generated image lifetime, material history, source/destination checks, recovery aggregation and New/Open job retirement. Existing bake probe call sites using direct Drawable preparation were inspected without execution.
- `git diff --check` passes. Prior worktree changes were preserved. No CMake, shaders, GPU backend or unrelated rendering APIs were modified.
- User validation pending: bake two instances of one model in different placements and verify independent maps/materials; apply, undo/redo, save/reopen and preview both results; run selected-subtree/whole-scene bake; replace or delete a receiver before application and verify stale results are rejected; cancel and New/Open during a job and verify retained resources retire safely.
- Standalone/offscreen checks: assemble a prefab containing instances with existing UV2; verify unique per-node output identities and image/metadata publication; verify owner-only UV2/model-copy diagnostics. Run the existing offscreen example with `--cached-model /absolute/path/model.bg2 --output-dir /a/new/directory` and the desired backend to exercise repeated resident loading and two appearances. No execution result is claimed.
- Recovery checks: open a scene containing a missing model/instance texture beside valid resources, review aggregated warnings and retained references, restore sources/retry, and verify incomplete export/copy failures preserve prior complete files and pending state. Standalone bake/output source failures remain strict to avoid publishing results from incomplete inputs.

### Limitations and handoff

Bake requirements for valid UV2, same-device resources, finite transforms and supported receiver materials still apply. Instances do not gain geometry editing, atlas generation or BG2 model-copy export. A source geometry replacement invalidates active bake source checks; retained native versions survive until job/frame retirement. Previously reviewed cleanup roots remain conservative as described in step 09. Runtime and memory behavior require user validation.

User acceptance for step 10: pending. Exact next step: [11 — Documentation after final implementation acceptance](11-documentation.md). Stop here until the user accepts the implementation and authorizes documentation.


### Follow-up: create instances from the selected owner's current appearance

The user confirmed that instance lightmaps are generated correctly, then reported that creating an instance from a selected BG2 owner after material edits used the cached original appearance. The user approved distinguishing explicit owner-based creation from cache-based creation.

Composer now initializes the bound instance with a deep copy of the selected owner's live material attributes and texture settings, including generated/unsaved lightmap source retention, drawable/local submesh transforms, names, groups, visibility and tracing eligibility. Geometry and BLAS remain bound to the same cache identity. Node placement follows the existing sibling/parent transform policy. Cache-based creation continues to use immutable original definitions; no source file or cache original is modified.

The entire appearance is prepared on the detached candidate before admitting the existing insertion command. Unavailable owner resources, incompatible transforms/topology or failed appearance preparation reject insertion and discard the candidate. Material-copy failures propagate through the throwing clone/assignment path, rather than silently accepting original defaults. The existing retained node command preserves the captured state for undo/redo.

No compilation or runtime tests were run, following the standing user instruction. `git diff --check` passes. User validation pending: load BG2, edit materials without saving, create an instance from the owner, verify matching appearance and subsequent independent edits, undo/redo creation, and save/reopen. Also check unsaved baked maps, per-submesh metadata/transforms, and unchanged original defaults when creating from the cache. Step 10 final acceptance remains pending; the next planned step is still 11 after acceptance.


## Step 11 — Final documentation

### Prerequisite and final implementation acceptance

The user confirmed that instance lightmaps work, approved copying the selected owner's current unsaved appearance, then explicitly stated that everything is correct and authorized the next step. Step 10, including that correction, is accepted. All implementation prerequisites for documentation are satisfied. Backend, packaging and offscreen execution coverage is not inferred beyond the user's reported results.

### Documentation delivered

- Added `doc/architecture/drawable_caches_and_instances.md`: sibling renderables, exclusive owner/shared identity, independent appearance, immutable originals, shared raster/BLAS, topology/replacement, explicit history/job/frame uses, document scopes, coordinated export, migration, cleanup reload transfer, bake/offscreen and future persistence boundaries.
- Added API pages for DrawableCache, DrawableRenderComponent, DrawableInstanceComponent and DrawableInstancesWindow; linked them from architecture, draw/scene/UI/DB and main documentation indexes.
- Rewrote DrawableComponent ownership/export documentation and corrected Text's sibling hierarchy and concrete/generic Node accessor descriptions.
- Updated SceneReferences, DrawResourceIO and SceneResourceManager for ordered instance component state, recoverable loading, one export per identity, overwrite/rollback, save reports, transient dependency roots/generations, preserved internal reload residency and fresh true New/Open scopes.
- Updated Composer and its guided tutorial for cache versus owner creation, copying unsaved material/lightmap state, independent editing, geometry restrictions, gizmos, history, complete saves and cleanup.
- Updated command-memento resource/geometry/flow/lifecycle documentation to describe shared geometry retention, independent appearances and generated sources, resource-aware memory accounting and retirement of old jobs on successful document replacement.
- Updated integrated/standalone bake, UV surface/executor, output writer and indirect-map documentation for per-instance receivers/images, existing shared UV2, owner-only model copies and strict source checks. Updated the offscreen application guide for `--cached-model FILE`.
- Reconciled the linked manual bake/cleanup checklists with resource sharing, stale source rejection, New/Open retirement, owner-appearance creation and nonpersisted cache protection. These are updated manual checks, not new executable tests.

### Verification and completion

- Markdown-only changes; no source, shader, CMake or API implementation changes in this step.
- `git diff --check` passes.
- Checked local link file targets in changed documentation/manual-check Markdown: no broken targets. Removed an obsolete UV surface implementation-plan link and linked the maintained lightmap architecture instead.
- Reviewed source-backed declarations, component JSON fields, save-report behavior and lifecycle policies, and searched for stale Text inheritance and unconditional bake-continuation descriptions.
- No compilation or runtime tests were run. No new graphics verification is claimed.

Steps 01–10 are user-accepted and step 11 documentation is delivered. Final documentation review remains with the user. Exact next step: **None**; no remaining implementation/documentation task is scheduled in this plan.


### Post-completion correction: Model Edit material patch compilation

The user reported a full-project compilation failure in Model Edit's material-patch action. That deferred callback still locked SelectionItem::mesh directly, yielding the const identity reference introduced for shared renderables, and passed it to MaterialModifier::apply(Drawable&, index).

The action now captures the complete SelectionItem and calls lockMesh() when its queued edit executes, matching the other Model Edit material actions. This yields validated mutable owner access while checking component/node membership, resource identity and submesh bounds. MaterialModifier signatures and instance read-only geometry restrictions remain unchanged. No const cast or API change was introduced.

No compilation or runtime tests were run, following the standing instruction. `git diff --check` passes. User full-project compilation is pending for this correction.
