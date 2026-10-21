# Manipulation API reference

**Headers:** `<bg2e/manipulation/SelectionManager.hpp>`,
`<bg2e/manipulation/GizmoComponent.hpp>`,
`<bg2e/manipulation/GizmoAndSelectionRenderer.hpp>`

See the [module overview](index.md) for resource lifetime boundaries.

## SelectionManager

Construct with `draw::Engine*`, then call `init()` (CPU selection needs no GPU
allocation; picking requires a live device at a stopped boundary). The manager owns the
selection state; selected objects are represented by weak, typed
`SelectionItem`s, so selection does not keep scene objects alive.

Common operations include:

```cpp
selection->setMultiSelectionEnabled(true);
selection->setClearSelectionOnEmptyPick(true);
selection->setPickPixelScale(0.7f, 0.7f); // drawable pixels per logical input unit
selection->pick(scene, logicalX, logicalY, additive);
selection->selectedItems();
selection->deselect();
selection->refreshSelection(); // after scene removal/replacement
```

`setPickPixelScale(x, y)` must match the relationship between input viewport
units and pixels in the picking image. For a 70% image from logical window
dimensions, use `(0.7, 0.7)`. A Retina logical-to-physical ratio may require a
different value when picking against physical pixels. Configure this before
calling `pick`; there is no implicit relationship with renderer resolution or
upscaler jitter.

### Draw picking

The picker extracts the visible render queue, assigns temporary
submission-local IDs, renders an RGBA8 ID image with D32 depth, and synchronously
reads back the requested pixel. These temporary IDs are not persistent
`SelectableComponent` IDs and must not be used for TLAS ordering or saved
identity. Calls require a stopped scene/frame producer boundary and must happen
outside command recording. Empty/out-of-range/disabled-root cases return false.
Texture alpha is not evaluated, so cutout holes remain pickable as geometry.

The viewport uses top-left logical coordinates. Scale after subtracting the
viewport origin; image dimensions are the rounded-up scaled viewport dimensions.
The draw example configures the same fixed 0.7 factor in renderer resolution
and `setPickPixelScale()` during delegate initialization. Its camera viewport
and input coordinates are logical, so on Retina those image extents can differ.

### Input capture

`mouseButtonDown`, `mouseMove` and `mouseButtonUp` integrate gizmo transforms
with application input. A `true` return from the input methods means scene
events may be propagated to camera controllers; a captured transform gesture
must remain inside the manager until release. `sceneInputBlocked()` reports
active gizmo capture. Click policy can be implemented by the application; the
example uses left-button down/up within ±2 logical pixels to pick and otherwise
forwards the gesture to the scene graph. Right drag pans and middle drag zooms
through `OrbitCameraController`.

## GizmoComponent

Attach a `GizmoComponent` to the same scene node as the component it represents.
For orbit cameras, keep `TransformComponent`, `OrbitCameraComponent` and
`CameraComponent` on that same node so the controller's transform is the camera
transform. Gizmos are runtime-only; their `clone()` creates a fresh transient
component. In non-`MainLoop` integrations, call `synchronizeResources()` at a
stopped scene boundary and clean up draw resources before device teardown.

The transform-gizmo facet is shown only for the current transform node and
requires a `TransformComponent`. `setCurrentTransform(node)` changes the one
visible transform gizmo. Visibility/scales and handle types can be queried or
configured through the static helpers declared by the header.

## Overlay

`GizmoAndSelectionRenderer` is an optional `draw::OverlayPass` supplied through
`RendererConfiguration::overlays`. It draws selection highlights and gizmos
after scene rendering. Retain the pass in the renderer configuration and
release gizmo resources before the draw engine is destroyed.

SelectionItem::mesh/lockMesh()/meshPtr() and SelectionManager::selectedMesh()
address draw::Drawable only. Locks validate node/component membership, resource
identity, submesh count and pick identifiers. Replacement invalidates stale items.
GizmoComponent::transformDrawable() returns its instance-owned draw resource;
cleanupDrawResources releases resources before device teardown.
