# Draw picking — M02-02 lead checks

No new executable or CMake target is introduced. Rebuild the library, paired
`draw/picking/picking` shaders and existing production/draw callers using the
normal build. Run these checks on Metal and Vulkan. The M02-07 manipulation
example will supply the final UI; the existing deferred example can host an
application-owned SelectionManager for these intermediate checks.

At a stopped scene/GPU boundary, create `SelectionManager(engine)` with a draw
Engine, attach SelectableComponent to drawable nodes and update the scene.
`init()` remains optional for draw CPU readiness; GPU pick resources are created
on demand. Route scene mouse events through the manager at that same safe
boundary, outside frame command recording. Do not call from a rendering callback
or concurrently with scene mutation/frame production. Release the manager and
scene resources before Engine teardown.

Coordinates and the camera projection viewport must use the same units. For
logical SDL mouse coordinates, use a logical viewport and call
`setPickPixelScale(drawableWidth / logicalWidth, drawableHeight / logicalHeight)`
with floating-point division. Default is (1,1). A caller using drawable pixel
coordinates and a pixel viewport keeps (1,1). Refresh on DPI/window changes.
Pass the unjittered projection, not an FSR-jittered frame projection. The viewport
origin is top-left in the input coordinate space; viewport width/height define
the separate pick image. No additional Y inversion or renderer resolution scale
is applied. Negative mouse coordinates must be rejected before uint32 conversion.

1. Pick each submesh of a nested transformed loaded draw model. Check node,
   submesh, `kind`, `mode`, `lockDrawMesh()` and fresh selectionIdentifier.
   Reorder transparent rendering; selection must still identify the same mesh.
2. Repeat at (1,1), (2,2), nonuniform scale, odd extents and nonzero viewport
   origins. Check all four corners, first/last pixel and outside edges. The
   selected location must agree with the displayed unjittered geometry.
3. Blank/outside/disabled/removed geometry yields false and an empty pickObject
   result, without returning the previous ID. Replacement of the drawable,
   CPU mesh, component or Selectable IDs invalidates old SelectionItems.
4. Overlapping selectable submeshes choose nearest depth. Hidden submeshes and
   hidden materials do not pick. Transparent/alpha-cutout surfaces use geometric
   picking, matching the production baseline (no texture alpha test).
5. Attach synchronized draw type gizmos: they pick their owner over scene
   geometry. A visible transform gizmo wins over both, self-occludes and ignores
   disabled handles. Disabling transform manipulation lets clicks pass through.
   Node-only groups without a gizmo remain tree-only.
6. Route button-down/move/up: a stationary click selects; camera drag does not.
   Additive click toggles a submesh; ordinary click replaces selection. Empty
   clicks honor clearSelectionOnEmptyPick. Transform hits start drag/capture;
   moving/releasing updates/ends the handle and does not select another object.
7. Repeat with normal/reduced output, bilinear/available FSR, query shadows on/off
   and no-RT mode. Pick coordinates must not follow internal renderer scaling;
   the next normal frame must keep valid rendering/resources/history behavior.
8. Check production SelectionManager/gizmo input and selection regressions.
   Picking is synchronous and may stall; no async or continuous per-frame
   picking performance claim is made.

Agent validation is nonexecuting static inspection only. Compilation, shader
compilation, input/runtime behavior and backend image checks belong to the lead.
