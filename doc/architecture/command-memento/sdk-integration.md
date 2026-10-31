# SDK integration

All integration is optional. Existing applications may keep widgets and gizmos
with current callbacks and no history. The generic core has no dependency on UI,
scene or draw.

## Reflection and property paths

Typed `ReflectionEditIntent` contains a `ReflectionPropertyPath`, registered
owner/property types and proposed `std::any` value. Composer resolves each path
again on execute/replay, so commands never store borrowed pointers to nested
components. Stale/read-only paths reject. Action intents have no implicit undo
contract. Polymorphic changes need an adapter retaining old and new instances.
Legacy callback APIs remain for non-history applications.

## Material editor migration

For command-owned edits install `DrawMaterialEditor::onEditRequested`. The
handler receives target and intent before live mutation and owns safe scheduling,
validation, snapshots and recovery. Typed mode does not run the editor's own
queue or changed notification. Call `commitInteraction` or `cancelInteraction`
as the host settles UI; cancel before rebind or cleanup. Both material editor
and component inspector expose `interactionActive()` so a host can verify that
cancellation has completed. An asynchronous owner returns false from Cancel
until restoration finishes and retains the session for a subsequent retry.
Composer queues CPU previews during the interaction and commits their already
applied final values; merely buffering until Commit would not provide viewport
feedback. In default mode,
`queuePatch`, texture settings and changed notifications remain usable. The
notification is not an undo request.

## Geometry and gizmos

`SelectionItem` identifies weak node, drawable component, draw mesh, submesh and
selection ID. Validate membership before use. Composer captures gizmo matrices
and adopts the preview at the graphics boundary. Independent applications may
continue using selection and gizmo tools without transactions. If they supervise
them, they own competing-writer pause/resume and cancellation boundaries.

`draw::Drawable::PreparedGeometry` prepares raster buffers and their dependent
BLAS before exchange and supports recovery. It is move-only; exchange retains
the displaced complete native version. Callers publish CPU data once per shared
Mesh and must stop competing writers and drain readers before exchange and
retirement. `accelerationBytes()` exposes retained BLAS/scratch bytes;
`resourceOwner()` supplies a lifetime identity for deferred release ledgers.
Composer supplies this scheduling, budgeting and retention policy; independent
SDK callers own those responsibilities. See [geometry](composer-geometry.md).

## Resource ownership

`base::Texture::retainSource` attaches a shared owner and declared byte size to
a source version. Copies preserve that owner, allowing generated files to
outlive their producer while live settings, snapshots, clipboard or redo still
reference them. This is not a global cache or a promise that arbitrary external
paths remain unchanged. Plugin code must remain loaded while commands or
deferred release callbacks can invoke its destructors.

See [public UI references](../../api/ui/EditInteraction.md) and
[origins/lifecycle](origins-and-lifecycle.md).
