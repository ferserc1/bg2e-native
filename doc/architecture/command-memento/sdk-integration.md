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
as the host settles UI; cancel before rebind or cleanup. In default mode,
`queuePatch`, texture settings and changed notifications remain usable. The
notification is not an undo request.

## Geometry and gizmos

`SelectionItem` identifies weak node, drawable component, draw mesh, submesh and
selection ID. Validate membership before use. Composer captures gizmo matrices
and adopts the preview at the graphics boundary. Independent applications may
continue using selection and gizmo tools without transactions. If they supervise
them, they own competing-writer pause/resume and cancellation boundaries.

`draw::Drawable::PreparedGeometry` prepares raster data before exchange and
supports recovery. It does not retain opaque ray-tracing acceleration
allocations; Composer rejects those cases. See [geometry](composer-geometry.md).

## Resource ownership

`base::Texture::retainSource` attaches a shared owner and declared byte size to
a source version. Copies preserve that owner, allowing generated files to
outlive their producer while live settings, snapshots, clipboard or redo still
reference them. This is not a global cache or a promise that arbitrary external
paths remain unchanged. Plugin code must remain loaded while commands or
deferred release callbacks can invoke its destructors.

See [public UI references](../../api/ui/EditInteraction.md) and
[origins/lifecycle](origins-and-lifecycle.md).
