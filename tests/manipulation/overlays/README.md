# Draw selection/gizmo overlays — M02-03 lead checks

No new executable is added until M02-07. Rebuild the library, existing callers and
paired `draw/selection_highlight/highlight` and `draw/gizmo/gizmo` shader libraries.
Use Metal and Vulkan where available. These are lead-owned compilation/runtime
checks; the agent performed static inspection only.

## Intermediate integration

Before renderer initialization, add the optional tool to its configuration:

```cpp
auto overlay = std::make_shared<bg2e::manipulation::GizmoAndSelectionRenderer>();
configuration.overlays.push_back(overlay);
```

RendererDeferred owns the extension lifecycle through shared configuration:
initialize, attachScene, pre-acquisition prepareFrame, update, post-final record,
resize and cleanup. Do not call `init()` or those methods separately for an
attached overlay. Do not share one overlay/graphics component between live
renderers. Keep the borrowed Engine alive through cleanup; release all external
tool/gizmo resources before Engine teardown.

For depth-tested/filled alternatives or custom shaders, construct the public
component before initialization:

```cpp
bg2e::draw::pipeline::MeshOverlaySettings settings;
settings.highlightDepthTest = true;
settings.typeGizmoDepthTest = true;
settings.typeGizmoLines = false;
auto graphics = std::make_shared<bg2e::draw::pipeline::MeshOverlayPass>(settings);
auto overlay = std::make_shared<bg2e::manipulation::GizmoAndSelectionRenderer>(graphics);
configuration.overlays.push_back(overlay);
```

Settings are immutable for a graphics lifecycle. A custom MeshOverlayPass may
replace graphics recording; a custom OverlayPass may replace the entire tool.
The default two shader pairs have the same PNUUT position location0 and vertex
push ABI: mat4 mvp + vec4 color, 80 bytes, Metal buffer1. Preserve that ABI in
shader replacements. Portable line width is one output pixel. Line/fill topology,
blend and scene-depth policies are configured separately for highlights/type
icons; transform handles remain triangle geometry with depth test/write enabled.

Route SelectionManager input at a stopped boundary as described in the
[picking checklist](../picking/README.md). A safe selection/visibility/material
change must request a scene frame in non-continuous mode. A fully paused scene
keeps its retained image until resumed; overlays are part of that image. A new
GPU mesh must exist before prepareFrame; do not replace topology inside frame
update/record. The prepare hook synchronizes GizmoComponent assets and builds
immutable cached edge meshes. Ordinary scene animation/transforms/live colors
are read after scene update without graphics allocation.

## Expected behavior

1. Attach SelectableComponent to a loaded nested multi-submesh object. Select
   each submesh independently/additively and clear selection. Only visible
   selected submeshes draw their albedo-colored triangle edges. Adjust
   setLineIntensity from0 to1; default0.8. Duplicate shared edges within each
   submesh should not increase alpha by being drawn twice.
2. Place another object in front. Default highlights/icons draw through scene
   geometry, matching production. With scene-depth testing enabled, occluded
   edges/icons disappear. highlightDepthBias controls a small normalized-depth
   offset to avoid wire edges fighting the surface. Depth is rerasterized from the visible queue at output
   resolution with the original projection; alpha textures are not tested.
3. Compare native/reduced/odd internal extents and bilinear/available FSR while
   moving the camera: overlays remain output-sized and unjittered. No overlay
   appears in G-buffer, shadows, HDR reconstruction or FSR history. Check output
   UNORM/sRGB formats and background/alpha preservation under blending.
4. Add light/camera/environment gizmos and change global type visibility,
   scale/opacity and light colors. Type icons use their configured wire/fill
   style and read live color. Ordinary drawable extraction/query shadows must
   exclude tool geometry.
5. Select a transform owner and expose the transform facet. Axis colors and
   active-handle colors must match live gizmo materials. Verify transform parts
   self-occlude and draw above both scene/icons, including when the owner lies
   behind geometry. Hidden scale handles must be hidden in both rendering and
   picking. Match M02-02 handle hit/capture/drag/release and camera blocking.
6. Replace/remove components, resources or the scene at safe boundaries; resize
   and replace the renderer (including surface format change). Check that old
   frames keep submitted geometry alive and only completed acquired slots are
   reused. No stale line geometry, dangling borrowed target or device errors.
7. Use empty/no-overlay configuration, no scene, no selected nodes, disabled
   nodes/ancestors, no RT and unavailable FSR. Ordinary rendering remains usable;
   headless configuration needs no manipulation objects.
8. Inject an OverlayPass that counts lifecycle/update/record calls, plus a
   MeshOverlayPass replacement. Verify unjittered context, output extent, slot
   indices, post-final ordering, resize, scene replacement and cleanup. Duplicate
   or null overlay registrations must fail. Partially failed initialization must
   remain cleanable; a recording callback must not leave a command scope open.
9. Run existing production editor selection/gizmo checks and the M02-01 CPU
   fixture. Legacy native draw/init paths and the new draw-mode init-only CPU
   contract remain available.

The line cache uses engine-standard triangle-list PNUUT mesh snapshots; immutable
GPU mesh identity is its key. In-place mutation of a GPU mesh's CPU mirror is not
an update mechanism: rebuild/replace the drawable mesh at a safe boundary. The
cache is instance-owned and contains no tracing geometry or device-global state.
