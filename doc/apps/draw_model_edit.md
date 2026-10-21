# Experimental draw model editor

`draw_model_edit` is the independent editor in `apps/draw/model_edit`, using the
draw renderer and its own application identity/preferences. The production
`model_edit` remains available. See the [application README](../../apps/draw/model_edit/README.md)
for launch/backend options and the [complete IO contract](../api/db/DrawResourceIO.md)
for supported import subsets and native sidecars.

## Workspace layout and menus

The main window follows the production editor workspace: a full-width toolbar sits
at the top, **Model Properties** is docked on the left, **Environment Options** is
docked on the right, and a status bar spans the bottom. These panels are arranged
by `bg2e::ui::Workspace`, so resizing and UI-scale changes preserve the layout.
The viewport remains between the two side panels. The application starts maximized.

The toolbar provides geometry operations, framing, duplication and quick toggles
for both side panels. **File** contains Open, Import glTF, Import OBJ, Save, Save As
and Quit. **View** contains camera reset/framing/best-view and selection-highlight actions.
**Window** contains checkable entries for both docked panels, plus UI Settings,
Graphics Settings and the separate **UV and Lightmap Baker** utility. Settings and
bake controls open from that menu; the document, properties and environment are not
separate floating windows. Keyboard shortcuts include Ctrl+O/S/Shift+S/Q, Ctrl+A
for best view and Shift+Space for selection highlight. The bottom status bar shows
the open document and its dirty state or the latest operation/viewpoint/bake message.

The shared renderer default selects global illumination and requests enabled GI
and reflections. Check their availability in Graphics Settings; unsupported
hardware/shaders keep the request visible and use the documented fallback. The
editor does not restore preferences implicitly, so an explicit settings import or
user change can override these defaults.

## Open and select

Open or drop a BG2/VWGLB, OBJ/MTL, static glTF/GLB or scene document. Model
properties lists document nodes and submeshes. Select a list entry or click the
viewport; enable **Add to selection** for multiple submeshes or viewport picks.
Dragging the camera does not select. Only lights expose gizmos; their transform
handles translate, rotate and scale the selected light. Captured drags continue
across UI panels. The editor converts
logical input coordinates to the camera viewport for DPI-independent picking.

Node names and transforms, submesh names, groups and visibility are editable.
Group and visibility changes affect the selected submeshes; displayed metadata
comes from the first selection. Four highlight levels control the selection
outline. Duplicate creates independent mesh/material/image resources; duplicating
the document root creates a group containing two independent copies. Removing a
child is supported; the document root is retained. Imported shared mesh identities
are expanded to independent editor resources before adoption.

## Materials and geometry

The native-free `DrawMaterialEditor` follows the typed selection. Edited fields
apply as partial patches; omitted material values stay unchanged. Six texture
sections expose the GPU preview, file picker/history, clear, UV set/scale,
applicable channel/inversion, color interpretation and sampler controls. Sampling
edits retain authored image/mip resources and apply only edited fields to each
target. Copy/paste snapshots actual GPU textures independently, including images
with no source file. JSON material patches resolve texture paths beside the patch.

The AO slot has explicit **Ambient occlusion**, **Baked AO** and **Baked GI**
interpretations. Choosing a new ordinary AO image or clearing the slot resets
baked interpretation. A baked interpretation requires an assigned image, makes
its settings linear and selects UV set 1 (the second UV set). Choosing UVs alone
does not create a lightmap. The six-slot material provides one indirect source,
so scalar AO and a baked RGB multiplier do not coexist as separate maps.

Whole selected meshes support Z-up to Y-up conversion, geometry centering,
placement on the geometry floor and centimeters-to-meters scaling. Geometry
reloads refresh GPU/tracing data and overlays. These operations change actual
geometry, while the node transform edits its placement in the scene.

## Environment and stage

Environment controls edit the document's Environment component. The existing
public draw generator builds a candidate before adoption; failed image generation
keeps its previous GPU snapshot. Sky visibility and normalized blur, six shipped
presets and custom HDR/image selection are available. Document lights have a
five-light editor limit, with type, color, intensity, shadows, source size, spot
angle and selectable transforms. Imported scenes are not silently truncated to
five lights; the limit prevents adding/restoring excessive editor lights.

The floor and fill light are editor fixtures with explicit visibility controls;
the floor also has a height control. They remain outside document model exports.
Disable stage fill to inspect document lighting alone. Camera field of view and
near/far planes are stage controls. Use **Frame selection** to fit the selected node or the whole document when nothing
is selected. **Analyze best view** renders a small synchronous set of camera
candidates on the selected node/document and applies the highest-scoring view. The
status reports the winning candidate, visible-submesh coverage, score and the
three most visible submeshes. A failed or empty analysis leaves the camera as it
was. UV and lightmap workflows are described below.

## UV2 and baked indirect maps

Open **Window → UV and Lightmap Baker** to inspect the selected drawable (or the
first drawable in the document) and its UV1/UV2 charts. **Generate UV2** creates a
new atlas and replaces the drawable at a stopped, GPU-safe renderer boundary; the
editor refreshes selection and previews after the reload. This changes document
geometry and marks the document dirty. Closing the utility cancels pending UV2
work. A UV2 map must exist before a baked map can produce useful texels.

UV2 triangle overlaps are warnings, shown below the UV preview, and never block
generation or baking. The bake retains all triangles even when multiple surfaces
write the same lightmap texels. A newly generated atlas can still warn: the atlas
generator checks chart separation at texel centers, while the overlap diagnostic
checks continuous triangle intersections, including within a chart.

Choose ray-traced AO or GI, resolution, completed accumulation samples, samples
per pixel, GI bounces, AO/ray limits, chart dilation and CPU export precision.
The baker records one sample into the draw frame command buffer and waits for that
submission to complete before advancing the exact progress count. It uses the
document graph, environment and lights; editor floor, fill light and camera
fixtures stay outside the bake. Unsupported ray-tracing profiles or invalid source
changes stop the job and leave no publishable result. **Cancel bake** retires the
in-flight frame before releasing the baker.

On completion, the editor automatically writes a temporary image and applies it
to the AO texture slot on every target submesh. RGB8 uses linear PNG; float output
uses `.bg2tex` to preserve HDR multipliers. The editor selects the explicit baked-AO
or baked-GI interpretation, linear sampling and UV2, and marks the document modified.
Saving through complete model/scene export publishes the applied image as a
native `.bg2tex` resource beside the document and references that durable copy.
The saved document does not depend on the temporary directory. The completed map
preview remains visible below the UV1/UV2 previews after automatic application;
starting another bake replaces it. Document replacement hides the previous result.

Environment presets save the Environment source/settings, document light poses,
floor height/visibility and the stage light properties, transform and enabled state
through complete scene IO. Restore
validates structure, image decoding and GPU generation before adoption. Light
poses are saved in world space and restored under an inverse root transform so
transformed document roots do not apply them twice. Presets replace environment/
light components, preserving other document nodes and custom content. Former light
nodes can remain as empty transform nodes and can be removed through properties.
The explicit personal default is `draw_model_edit_environment.json` in the app
settings directory. Nothing automatically reads the production editor's env.json.

## Saving and lifetime

Edits run before acquisition, guarded by weak node/resource identities and the
owning document generation. Material widgets also check binding and renderer
generations. Successful and partially failed edits mark the document dirty,
request redraw and invalidate histories. A stale edit does not modify a new
model. Diagnostics remain visible after partial failure.

Save flushes queued edits before snapshotting. Open/Quit/close also flush pending
changes before their dirty checks. Save/Discard/Cancel protects replacement and
exit. Failed or cancelled imports retain the current document. Saving unresolved
texture updates fails rather than attaching old GPU pixels to new intended
settings. Graph edits or environment/light components may require saving as a
scene instead of a single BG2 model.

File menu shortcuts are Ctrl+O, Ctrl+S, Ctrl+Shift+S and Ctrl+Q. The Model
Properties and Environment Options docked panels toggle from the toolbar or Window
menu. UI Settings and Graphics Settings open from the Window
menu. The Graphics window uses live reflected renderer/effect/upscaler settings with
explicit save/restore/import, rather than copied production RT toggle enums.
Widget previews retire through the draw UI session's completion-aware registry;
application cleanup detaches bindings before renderer and stage resources.

The [manual checks](../../tests/draw/editor_tools/README.md) describe actual
project-lead validation. Source/static review is not compilation or Vulkan/Metal
execution evidence. The project lead explicitly reports correct rendering with
zero lights; other runtime/package coverage remains unspecified. No build or
application run was performed by the agent.

### Graphics settings and focused light editors

Graphics Settings contains only graphics quality controls. Profiling and diagnostic
controls are absent; neither debug controls nor scene environment/lights are
included in its preference documents.

Environment Options can switch an image environment back to Procedural sky, even
when the previous source was a custom file. It shows the stage light first and
then every document light. Each light uses reflected TransformComponent and
LightComponent properties, without exposing the node's other components or the
full scene/component inspector. Edits operate on temporary component clones and
are queued to the renderer's stopped update boundary. Environment presets retain
the stage light's transform and light parameters as well as its enabled state.

### Light-only gizmos

The model editor prepares gizmo components only on light nodes, including the
stage light. Selecting a light can show its transform handles. Selecting a model,
camera or environment node does not show a transform or type gizmo. Object
selection/highlighting and authored model transforms remain available; hiding
editor handles does not alter the model's saved pose or geometry.

### Removing environment lights

Each light editor, including the initial stage light, has a **Remove light**
button. Removal runs at the stopped editor update boundary and removes the light
component and its editor gizmo. Other node components and children remain intact.
There is no minimum light count: every light may be removed. Adding a light remains
available afterwards, subject to the existing five-document-light limit.
Environment presets preserve an absent stage light and empty document light lists.
The initial stage light starts at `(2, 2, 2)`. **Add light** places each new light
at independent uniform random local coordinates: X/Z in `[-3, 3]`, Y in `[0, 3]`.
An imported/transformed parent affects the resulting world-space location.

For a sequential walkthrough, see the [model editor tutorial](../tutorials/draw/model_edit.md).
For supported development generators, shader/assets deployment and distribution,
see the [application README](../../apps/draw/model_edit/README.md) and
[packaging guide](../../scripts/PACKAGING.md).
