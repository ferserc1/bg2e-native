# Editing, lighting and baking a model

This tutorial uses the delivered `model_edit` application. It assumes basic
familiarity with opening files and entering numbers. For implementing a C++ draw
application, begin with [tutorial 10](10_deferred_renderer.md); for the baker APIs,
use [tutorial 12](12_lightmaps.md).

The editor uses the production `bg2e::draw` rendering path. Application preference identities are preserved.

## 1. Launch the editor with its resources

Use the configured repository build's `model_edit` executable. On macOS,
launch the app bundle or its `Contents/MacOS/model_edit` executable so its
bundled engine, shaders and assets can be found. Metal is available on macOS:

```sh
./draw_model_edit --backend metal
```

To select Vulkan explicitly, use `--backend vulkan`. `--help` prints the accepted
arguments. A backend selection requires that backend to be available on the host;
it does not install its SDK or runtime. Use Xcode on macOS for working native file
dialogs, Ninja on Linux, and Visual Studio 17 2022 on Windows. The configured build
needs a Vulkan SDK. See the [application deployment instructions](../../../apps/model_edit/README.md)
and [packaging guide](../../../scripts/PACKAGING.md) for resource distribution.

You should see a viewport between Model Properties and Environment Options, a
full-width toolbar above them and a status bar below. The startup cube is a
fixture; open a document before editing or baking. Left-drag orbits the camera,
right-drag pans, and the mouse wheel zooms.

## 2. Open a model and choose what to edit

Choose File → Open or drop a supported file onto the editor. BG2/VWGLB, OBJ/MTL,
static glTF/GLB and supported scene documents have draw-aware loaders. Imports
are loaded as candidates before replacing the current document. A cancelled or
failed import keeps the current document.

Select a submesh in Model Properties or click it in the viewport. Enable Add to
selection to edit multiple submeshes. The highlight identifies the selection;
object transform gizmos are deliberately hidden in this model editor. Light
nodes can show their own marker and transform handles.

Use Frame selection to fit the selected node, or the document when nothing is
selected. Analyze best view evaluates candidate camera poses and reports visible
submeshes and a score. It changes the camera, not the model geometry.

## 3. Edit materials and geometry

Use the material editor to change the selected submeshes. Each texture section
has a preview, file selection, UV-set/scale and sampler controls. A changed field
is applied as a partial edit: unrelated properties retain their previous values.
Copy/paste creates independent GPU texture resources.

For ordinary texture AO, choose Ambient occlusion in the AO section. Baked AO and
Baked GI use the same slot with explicit interpretation; these modes require an
assigned linear image and the second UV set. Selecting that UV set by itself does
not bake anything.

Geometry actions can center meshes, place them on the geometry floor, convert
Z-up to Y-up, or convert centimeters to meters. They modify geometry. Node
transforms instead change placement. The dirty indicator means a save is needed.

## 4. Set up the environment and lights

In Environment Options, choose a shipped environment image or Custom HDR. Choose
Procedural sky to return to generated illumination after using an image. Sky
visibility and sky blur control its viewport appearance.

The initial stage light starts at `(2, 2, 2)`. Its editor appears before the
document lights. Add light creates a document light at random local X/Z positions
between -3 and 3 and a Y position between 0 and 3. The existing limit is five
added document lights; imported scenes are not silently truncated.

Expand a light's Transform and Light sections. These are generated from the
reflection metadata of TransformComponent and LightComponent; the editor does
not expose unrelated components of that node. Translation and scale fields have
no application range: drag horizontally or Ctrl+click/double-click to type an
exact number. Rotation keeps its bounded angle control. Light properties include
its type, color/intensity and reflected shadow/spot parameters.

Remove light removes that light and its gizmo while retaining other components
and children. You may remove every light, including the initial stage light.
The project lead has reported correct rendering with no lights. This tutorial
adds no further renderer requirements for that configuration.

Save environment preset records the environment, document-light poses, floor
controls and the stage light's parameters/transform or absence. Restoring a preset
replaces those lighting components and preserves other document content. These
presets are separate from graphics-quality preferences.

## 5. Choose graphics quality and inspect diagnostics separately

Open Window → Graphics Settings. Select the indirect-lighting method and adjust
global shadows, AO/GI, reflections, temporal accumulation, denoising, color,
upscaling/resolution and final processing. The renderer defaults to GI with GI
and reflections requested enabled. Availability depends on backend/device/shaders;
a request does not prove that tracing is available.

Show unavailable settings keeps unsupported choices visible when needed. Save
preferences and Import/Export affect graphics-quality values. Environment and
light controls remain in Environment Options.

Graphics Settings contains only graphics controls. Preference serialization
excludes diagnostic selections, and restore skips legacy debug entries.

## 6. Generate UV2 and bake

Select the model drawable and open Window → UV and Lightmap Baker. The utility
keeps both UV1 and UV2 previews visible. Generate UV2 creates a lightmap atlas at a
safe renderer update boundary and marks geometry modified.

An overlap message is a warning. Generation and bake keep all triangles even
when they overwrite the same atlas region. Malformed indices or invalid UV
coordinates still prevent a valid job; overlap alone does not.

Choose ray-traced AO or GI, resolution, completed samples, samples per pixel,
ray settings, dilation and CPU precision, then press Start bake. Progress advances
after submitted work completes. Cancel bake retires work before releasing its
resources. Unsupported tracing or invalidated document sources cannot publish a
completed map. The bake root is the document; stage floor/fill/camera fixtures
are excluded.

On completion, the editor writes a temporary linear PNG for RGB8 output or a
native float texture archive for RGB32F, applies it to every target submesh using
UV2 and the chosen baked interpretation, and marks the document modified. You do
not need a separate Apply button. The completed-map preview remains below the
UV previews and shows the applied image. A new bake replaces that preview.

## 7. Save and reopen the result

Use File → Save As. Choose BG2 for a representable single model, or a supported
scene format such as JSON to preserve a graph and its components. Complete draw
resource export publishes actual GPU pixels, including the baked image, as native
`.bg2tex` resources beside the saved document. The document references the final
resources and does not depend on temporary bake paths.

Reopen the saved document and inspect the materials and applied map. Verify this
with the application's deployed resource tree, not an isolated executable. Native
archives preserve HDR values and authored mip/layer data; reference-only scene IO
does not materialize those resources. See [complete draw resource IO](../../api/db/DrawResourceIO.md).

Before replacing a dirty document or quitting, Save/Discard/Cancel lets you choose
what happens to edits. Resource errors appear in the status bar. The application
queues edits before acquisition, checks document/renderer identities, and retires
submitted GPU work before releasing resources; these APIs make delayed UI changes
safe without giving this model editor a complete scene-component inspector.

## Source and validation

The [application guide](../../apps/draw_model_edit.md) details supported subsets
and lifetime rules. The implementation is in [EditorUi.cpp](../../../apps/model_edit/src/EditorUi.cpp),
[EditorTools.cpp](../../../apps/model_edit/src/EditorTools.cpp),
[EditorBaking.cpp](../../../apps/model_edit/src/EditorBaking.cpp) and
[StageScene.cpp](../../../apps/model_edit/src/StageScene.cpp).
The [manual editor checks](../../../tests/draw/editor_tools/README.md) belong to
the project lead. This tutorial was checked against source, not executed as a new
Vulkan/Metal or packaged-runtime validation run.
