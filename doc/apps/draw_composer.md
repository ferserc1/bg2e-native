# Composer

`bg2e_composer`, implemented in [apps/bg2e_composer](../../apps/bg2e_composer/README.md), edits complete scenes through `bg2e::draw`. The standalone draw lightmap generator is not delivered by Composer.

## Launch and deployment

Run `bg2e_composer [--backend vulkan|metal]`; `--help` lists accepted arguments. Use a backend available on the host. The existing CMake target is `bg2e_composer`; the shared SDL bundle helper deploys engine shaders and assets. Launch with that resource tree, or the macOS bundle and its `Contents/MacOS/bg2e_composer` executable. Native file dialogs require Xcode on macOS; use Ninja on Linux and Visual Studio 17 2022 on Windows. The repository configuration requires `VULKAN_SDK`. See [packaging](../../scripts/PACKAGING.md); source routing alone does not establish native package launch coverage.

Graphics preferences use `draw.bg2e_composer.graphics`, separately from document lighting and former application preference scopes. Window → Graphics Settings exposes active renderer settings; changes persist automatically. JSON import/export is optional. Window → UI Settings controls foreground/background scene FPS independently of UI presentation; a zero cap is unlimited.

## Performance collection and recording

Window → Performance opens live CPU/GPU charts without creating a capture file.
Start Recording selects a JSONL destination; Stop Recording ends that file session.
Closing/reopening the window releases/reacquires only its live collection lease,
so closing the window does not stop an active recording. MainLoop shutdown stops
recording before normal teardown. Graphics Settings has no profiler controls;
Model Edit and the headless tools do not expose this Composer window.

GPU results are polled asynchronously. Vulkan pipeline-boundary intervals and
Metal encoder envelopes have different granularity; overlapping submissions and
nested CPU scopes must not be summed as whole-frame time. See
[profiling ownership and evidence](../architecture/cpu_profiling.md).

## Scene editing

Model Properties on the left edits selected drawable submeshes/materials. Scene on the right contains the hierarchy and node inspector. There is no separate Environment Options panel. Select authored camera, light and Environment nodes to edit them. The registered Add Component dropdown and component Remove controls use the shared inspector. Chain controls edit model joints. Create Node and Create Primitive add children below the selected node or document root; cube, sphere and plane carry editable transforms.

Shift-click extends selection in the hierarchy, viewport and submesh list. Structural actions use the primary node. Drag reparenting preserves the local transform. A removed Transform is not silently recreated by tools. Scene edits require at least one enabled camera and at most eight enabled authored lights. Only the active camera's authored orbit controller receives viewport input; static cameras still render. Framing and viewpoint analysis require supported controller/parent transforms and report unsupported cases. Floor/fill fixtures start disabled and are excluded from authored output.

On fresh Open, the first enabled depth-first Environment is selected. Later additions cannot replace a valid active component. Removing it restores renderer defaults without promoting previously ignored components; a newly eligible component can fill an empty selection. Select the active node to edit procedural RGB, assign floating-point `.hdr`/`.exr`, or Clear its HDR reference. Stored colors survive HDR assignment and become visible after clearing. Browse cancellation preserves the path. Errors remain visible in status/inspector and retain last-good GPU resources. See [Environment selection and generation](../api/draw/Environment.md).

## Documents and imports

File Open replaces the document; New/Close creates an empty camera-bearing scene. Save/Discard/Cancel protects dirty replacements and quit. Open Scene reads JSON on a worker, validates/deserializes on the main thread, then realizes one component per frame under a visible Loader. Individual image/model uploads and environment generation can still stall a frame. Publication waits for a complete valid candidate; ordinary failures preserve the old document and show an error.

Model/prefab imports insert below selection or root, retaining the scene save path. Supported inputs include BG2/VWGLB, OBJ/MTL, static glTF/GLB and scene JSON prefabs. CPU parsing is asynchronous; main-thread realization is synchronous. The local queue allows 16 waiting jobs. Cancellation and document replacement reject stale publication, but cannot interrupt a third-party parser or an active realization call. External referenced files must remain available until realization finishes.

Window → Import Settings controls local units/Z-up conversion and the localhost HTTP service, enabled by default on port 8643 with preference scope `draw.bg2e_composer.import`. Another Composer using that port causes a visible bind failure. See the [HTTP protocol](composer/import_service.md) and [draw import lifecycle](composer/draw_import_lifecycle.md).

Unregistered/untyped or malformed component entries warn and are skipped; subsequent valid graph data continues loading. Registered parser, constructor and resource failures remain errors. Typed save omits skipped representations; [raw JSON IO](../api/db/SceneReferences.md) preserves them.

Save/Save As uses the Name/Name.json layout and [complete resource IO](../api/db/DrawResourceIO.md), including GPU-only HDR/mip images in native `.bg2tex` sidecars. Keep the document and resources together. Reference-only save does not export pixels. Export Selected Model(s) writes directly selected models and joints without descendants or node placement; each item commits separately, so earlier exports survive a later failure.

## Integrated scene baking

The UV/Lightmap window targets its preview drawable, selected hierarchy subtrees or all document drawables. Generate UV2 where needed. Genuine overlaps warn without excluding targets. One live baker and one submitted frame run at a time against frozen sources; all targets complete before material preparation/adoption. Cancel, import or source edits invalidate pending work. Required RT capability failures leave ordinary editing usable; no-RT rendering does not promise RT baking.

Select a completed target for its own map preview. Applied maps use linear RGB UV2 with explicit baked AO/GI metadata. RGB8 uses temporary PNG sources; float output uses native HDR archives. Applied images and temporary sources remain available for complete Save. Interactive adoption rejects values outside finite half-float range. A failure after material mutation can leave partial dirty edits and retained sources; this is separate from complete DB publication and does not provide the [standalone batch](../api/bake/StandaloneBakeBatch.md)'s protected whole-generation publication guarantees.

Tracing tables use exact device budgets up to 16384, including sampler limits. Excess image/sampler pairs warn and use neutral texture fallback while keeping geometry and raster materials. Direct query shadows also retain the scene on overflow. Materials remain independently owned. See [tracing resources](../api/draw/TracingSceneResources.md) and [query shadows](../api/draw/RendererDeferred.md).

## Implementation and validation

[DocumentIo.cpp](../../apps/bg2e_composer/src/DocumentIo.cpp), [SceneEditor.cpp](../../apps/bg2e_composer/src/SceneEditor.cpp), [ImportActions.cpp](../../apps/bg2e_composer/src/ImportActions.cpp) and [EditorBaking.cpp](../../apps/bg2e_composer/src/EditorBaking.cpp) own application policy. Shared graph/root/clone hooks are described in [scene API](../api/scene/index.md). No engine API depends on Composer's import queues.

The [guided workflow](../tutorials/draw/composer.md) and manual [IO](../../tests/draw/composer_io/README.md), [import](../../tests/draw/composer_import/README.md) and [baking](../../tests/draw/composer_baking/README.md) matrices guide lead validation. M07 implementation/corrections are accepted by continuation; exact backend, HTTP and package coverage is unspecified. Documentation checks are static, with no new runtime run implied.
