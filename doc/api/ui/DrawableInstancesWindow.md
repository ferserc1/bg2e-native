# DrawableInstancesWindow

Header: `<bg2e/ui/DrawableInstancesWindow.hpp>`; namespace `bg2e::ui`.
Composer opens the window from **Window > Drawable Instances**.

`configure()` accepts host callbacks for snapshot/availability, load, unload,
create from resource, create from owner, rebind, remove and status. Callbacks queue
host IO or scene commands; cache/native mutations occur at the stopped update
boundary. Applications call `draw()` through the normal UI lifecycle. No ImGui
headers/types are required by the public interface.

`DrawableBrowserSnapshot` contains document generation, entries, preferred handle
and selected-component inspection. Entries expose labels, source/export paths,
geometry revision, scope, logical usage and an unload diagnostic. Independent
submesh inspection includes names/groups, material/image descriptions, visibility
and read-only model-local matrices. The drawable-wide matrix display is for
model owners. Use the node's Transform component to place an instance. These DTOs do not own caches, native realizations or usage
leases merely because the browser is open.

Composer cache load/unload is nonhistorical and does not mark the document dirty.
Unload is unavailable while owners, instances, history, jobs or frames retain the
resource. Loading changes cache generation and invalidates disk cleanup review.
Scene creation/rebind/removal are undoable commands. Cache-based creation uses
original definitions; owner-based creation copies current unsaved appearance.

The component/material inspector edits independent instance appearance, with
geometry/UV2 controls restricted to owners. The Drawable Instance type gizmo is a
small pair of offset wireframe cubes, hidden by default. Its visibility, size and
opacity are configured in UI Settings; node transform handles remain separate.

See [Composer workflow](../../apps/draw_composer.md#drawable-instances) and
[DrawableCache](../draw/DrawableCache.md).
