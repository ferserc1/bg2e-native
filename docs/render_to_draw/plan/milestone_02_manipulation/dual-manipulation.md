# M02-01 delivered manipulation contracts

Date: 2026-10-15. Source baseline `4d58ce60f568dca2e977b27ae5161a8b94913dbd`.
M01-16 explicitly validated; M01-17 completed with separate acceptance waived by
the lead, who authorized immediate M02-01. Production behavior remains the reference.

## Modes and optional attachment

`manipulation::ToolExecutionMode` (ToolExecutionMode.hpp) distinguishes Uninitialized,
Production and Draw without colliding with DrawableComponent's resource-mode enum.
SelectionManager and GizmoComponent accept non-null production/draw Engine pointers.
SelectionManager::executionMode and GizmoComponent::manipulationMode report the
fixed tool mode. GizmoAndSelectionRenderer has production init(engine, samples)
and draw init(engine), plus executionMode; initialization is explicit and rejects
mode replacement. Engines/devices are borrowed and must outlive tools/resources.

Tools are application-owned and optional. Attach Selectable/Gizmo components to
nodes and create a SelectionManager only when editing is needed. No Renderer
base/member or headless entry point requires them. Node's exact-type component
lookup keeps GizmoComponent geometry separate from ordinary DrawableComponent
extraction, so normal lighting/query geometry does not include tool meshes.
No new GPU/native implementation is introduced in manipulation. Existing legacy
Vulkan implementation remains intact through coexistence.

## Selection identities, lifetime and replacement

SelectionItem retains weak node/component and either weak scene::Drawable `mesh`
or weak draw::Drawable `drawMesh`, with mode, submesh, kind and selectionIdentifier.
The original five aggregate fields remain first. Exported out-of-line methods
validate membership and resource identity. `lockMesh`/`lockDrawMesh` retain typed
ownership for callers; raw meshPtr/drawMeshPtr are borrowed. The wrong typed
resource returns null. selectedMesh remains production; selectedDrawMesh is draw.

Regular selections expire after node deletion, component detachment/replacement,
resource replacement, submesh loss or regenerated selection IDs, even if another
owner keeps the old component/resource alive. Node/type-gizmo picks do not require
a drawable. `refreshSelection()` synchronizes CPU identity/count, prunes stale
entries and unique selected nodes, and notifies once when the state changed.
Call it at scene-update boundaries before consuming selectedItems/selectedNodes
(the collection accessors preserve their reference-returning API). Typed primary
getters reject stale geometry immediately rather than convert it into node-only
selection. Duplicate selection does not notify; multi-selection/dedup/remove and
production callback paths remain shared. No background/thread-safe mutation is
promised. Queries may synchronize the CPU Selectable state; no GPU work occurs.

SelectableComponent uses resource mode to select drawable()/drawDrawable() and
never calls the production accessor on draw geometry. A new drawable identity,
CPU mesh identity or changed submesh count resets every selected bit and allocates
fresh globally unique per-submesh IDs. In-place edits retaining mesh identity and
submesh count retain slot identities; replace the mesh or clear selection when
reinterpreting those slots. Out-of-range identifier/isSelected return zero/false.
ID wrap throws instead of colliding with zero/another object. Clones preserve
common priority/serialization intent but receive fresh runtime IDs. Graphics-free
deserialize reads common attributes and clears transient state; serializeReferences
writes common type/priority only. Production deserialization remains available.

SelectionManager rejects opposite resource modes and foreign loaded draw engines.
CPU draw selection works without initialized GPU resources. Draw `pick`/`pickObject`
explicitly reject the pending graphics path, preserving selection; M02-02 replaces
those guards. init() in draw mode establishes CPU readiness only. Queue-local draw
object IDs remain distinct from global Selectable IDs; M02-02 must map a retained
submission snapshot and reject stale scene/resource identities.

## Gizmo resources and safe updates

Draw gizmos load existing BG2 assets or procedural sphere into draw::Drawable,
retain names/groups and unlit axis colors, disable tracing before realization,
and keep instance-owned caches. This intentionally avoids a new static draw
cache/lifecycle system: no device crossing, mutable handle sharing or clone sharing.
Production cache/asset paths remain available. transformDrawable is production;
transformDrawDrawable is draw; wrong mode throws. Transform identifiers, shared
visibility/settings, drag math and typed live handle-color restore are preserved.

`synchronizeResources()` runs at a caller-owned stopped scene/GPU boundary and
works independently of MainLoop. Ordinary update queues one weak component callback
through MainLoop::safeUpdateScene; callbacks also validate weak node lifetime and
current component membership. Removed/replaced nodes/components are not touched,
even when external owners keep them alive. Weak owner tracking also prevents a
retained current gizmo from returning a deleted node. No buffers are built during
command recording. `cleanupDrawResources()` is an explicit stopped-boundary release;
release scene/tool resources and any external owners before Engine teardown.

Draw overlay init selects mode only; native draw rejects that mode. The actual
public draw overlay recording/surface/camera attachment belongs to M02-03. There
is no new picking shader/pass, native readback, overlay pipeline or example here.
The maintained M01 documentation deliberately does not present these future
M02 product workflows as implemented.

## Static checks and project-lead checks

Reviewed local includes/signatures/overloads and production model editor/composer/
renderer consumers, schema dispatch, exact component lookup, ownership and safe
callbacks; checked delimiters and local Markdown paths. Scoped whitespace checks
pass. Unrelated concurrent draw/UI source/header edits are preserved and excluded
from scoped checks; whole-tree whitespace checks reported trailing blanks in
those separate edits, which this work did not change. No CMake file changed. No compiler, shader compiler,
fixture or runtime executable ran. The instructed skills.md is absent from this
checkout; existing architecture.md and plan architecture/review sources were read.

[Manual CPU fixture and backend/production checklist](../../../../tests/manipulation/dual_mode/README.md)
cover duplicate callbacks, selection replacement/deletion, fresh IDs/schema,
cross-mode rejection and initialized-mode objects. Lead should compile the library
and existing draw/production callers, run the CPU fixture, then check stopped-boundary
gizmo asset creation/cleanup on Vulkan and Metal and production interaction regressions.
No new executable is registered. M02-01 awaits lead acceptance; M02-02 is next.
