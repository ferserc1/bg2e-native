# Experimental draw model editor

Target: `model_edit`. Application ID: `org.bg2e.apps.draw.model-edit`.
Graphics preference scope: `draw.model_edit.graphics`.

The application supplies a draw-only model editing workflow in a maximized window. Its
Workspace matches production model_edit: full-width toolbar, docked Model Properties
and Environment Options panels, and a bottom status bar. The app uses a deferred
renderer, procedural startup cube/floor, generated environment, stage light and
orbit camera. UI settings and Graphics settings open separately from the Window menu. The
graphics facade connects before first renderer attachment so replacement and
resize can rebind its catalog safely. Values do not restore implicitly; explicit
settings import/restore uses the existing queued facade. The floor is an editor
fixture and is excluded from the integrated bake document graph.

Build the registered target through the existing configured repository build;
use the supported platform generator and Vulkan SDK. Engine paired draw/bake
shaders, assets and native runtime dependencies use `bundle_app_sdl`, including
shader-only bundle refresh. No application-owned shaders or legacy texture_gi
shader copy is required by this startup. Do not launch a copied executable without
its deployed resource tree.

```sh
./model_edit
./model_edit --backend vulkan
./model_edit --backend metal
./model_edit --help
```

Metal requires macOS. Otherwise the EngineConfig platform default applies. Use
the binary inside `model_edit.app/Contents/MacOS` on macOS, or
`model_edit.exe` on Windows. Unsupported/duplicate options or backend failures
return nonzero. The target is `model_edit`; its existing app identity/settings remain intact. New graphics preferences do not read or write the production
render preferences or environment `env.json`.

Drag left to orbit, drag right to pan, and use the wheel to zoom. Reset camera is
a generation-checked stopped renderer update. Open UI and Graphics settings from
the toolbar Window menu. Graphics settings lets you inspect requested effects,
resolution and optional upscalers. Environment Options owns scene lighting. With no RT or FSR, ordinary raster/IBL and
resampling remain the baseline. Native backend availability still requires a real
Device and deployed shaders.

The renderer starts in GI mode with GI and reflections requested enabled, matching
the shared draw renderer defaults. Actual availability depends on the selected
backend/device; inspect Graphics settings for status. The application does not
restore settings implicitly, and explicit preferences/imports remain authoritative.

The startup cube is a fixture, not an opened document. Open or drop BG2/VWGLB,
OBJ/MTL, glTF/GLB or JSON/BG2SCENE/BG2E scene files. A candidate is fully loaded
before replacing the current model. Import formats are dirty until saved in a
native format. Save and Save as publish complete native resources, including
GPU-only images. Use JSON scenes for graph/component preservation; single-node
BG2 export rejects unrepresentable graph state. Save/Discard/Cancel protects dirty
documents on replacement and close; cancelled dialogs and failed IO retain the
current document. Status messages report errors without terminating the editor.

See [complete resource IO](../../doc/api/db/DrawResourceIO.md) for import
subsets, source/relative-path behavior, native HDR/mip/sampler archives, sidecar
publication and production compatibility. Preserve sidecars when moving a saved
document. Old resource generations are retained. Selection, gizmos, material/submesh/geometry tools, environment/light/preset
controls, bounds framing and best-view analysis are implemented by M06-03/04.
Frame selection fits the current selection or whole document. Best-view analysis
renders 72 candidate poses at up to 48 pixels along the longest image dimension,
preserving viewport aspect at a stopped boundary. It reports the winning score,
visible submeshes and top contributors. The [editor guide](../../doc/apps/model_edit.md)
describes these workflows, stage fixtures, queued dirty-state guards and texture
sampling/indirect controls. **Window → UV and Lightmap Baker** previews UV1/UV2,
regenerates UV2 at a safe boundary, runs integrated AO/GI accumulation, and applies
the completed image automatically from a temporary filesystem source. The
generated-map preview remains visible alongside UV previews, and saving publishes
the applied map with the final document resources.

The [feature inventory](../../docs/render_to_draw/plan/milestone_06_model_edit/application-inventory.md)
accounts for remaining workflows. Project-lead checks: build/run model_edit on
available backends, settings/orbit/resize/minimize/cleanup, file dialogs/drop,
unsaved prompts, failed operation recovery, complete save/reload and independent
production editor preferences. The [resource IO probe](../../tests/draw/resource_io/README.md)
adds GPU-only HDR/mip/metadata/rollback checks. No agent build, shader compilation,
probe or application run was performed.


The draw editor now uses bg2e::ui::Workspace to match model_edit panel positions,
toolbar/menu hierarchy and status layout. The Window menu toggles the docked panels
and opens separate UI/Graphics settings windows. File/View/Window actions and the
geometry toolbar preserve the familiar workflow.
[Editor tool checks](../../tests/draw/editor_tools/README.md) include the
unexecuted manual probe for partial patches, independent clones, sampler image
retention and stale document rejection. Compilation and interaction/backend checks
remain project-lead work; no agent runtime evidence is claimed.
