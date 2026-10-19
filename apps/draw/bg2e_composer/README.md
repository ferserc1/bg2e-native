# Draw Composer baseline

## Composer panels

Model Properties is the left panel for the selected drawable/submeshes/materials.
Scene is the right panel, split into the scene tree and selected-node inspector.
Add Component uses the production ComponentInspector's registered-type dropdown;
component headers expose Remove. There is no separate Environment Options panel:
edit environment, lights and cameras by selecting their authored scene nodes.
Create Node is available in the toolbar menu. The inherited stage floor/fill
fixtures start disabled, and tools do not recreate removed Transform components.

## Integrated scene baking (M07-05)

The UV/Lightmap window can bake its preview target, the selected SceneTree
subtrees or all document drawables. Generate UV2 on targets as needed; genuine
overlap warns without excluding geometry. Targets run serially against the same
sources and all results finish before automatic material application. Select a
completed target to inspect its own lightmap preview. Cancel/source edits/imports
invalidate pending batches. Required RT capability failures leave editing usable.

Applied maps use linear UV2 and explicit baked AO/GI metadata. Temporary PNG
(RGB8) or native HDR archive sources remain alive for complete scene Save/Save as;
successful save publishes them with the scene resources. HDR outside the finite
half-float display range rejects application. Partial material adoption failures
are reported and leave the document dirty. See the
[manual checks](../../../tests/draw/composer_baking/README.md).

M07-01 creates the separate `draw_bg2e_composer` application. The production
`bg2e_composer` remains available. This is the startup/editor foundation, not yet
Composer feature parity.

Run `draw_bg2e_composer [--backend vulkan|metal]` from the normal product directory
(or its macOS bundle). `--help` describes backend selection. Use the repository's
normal native generator and Vulkan SDK configuration; the shared SDL bundle helper
deploys engine shaders/assets. No application-owned shader copy is needed.

The maximized Workspace contains Scene Properties, Environment Options, toolbar
and status bar. Window opens Graphics Settings, renderer diagnostics and UV/lightmap
utilities. Graphics preferences use `draw.bg2e_composer.graphics`; environment
defaults use `draw_bg2e_composer_environment.json`. The application ID is
`org.bg2e.apps.draw.bg2e-composer`.

The initial editable `Scene` contains a cube and an authored orbit camera. Floor
and fill light remain editor fixtures. Scene open validates an enabled camera;
model open creates a new camera-bearing document. Model drops and File > Import
Model or Prefab insert a subtree below selection or the root, retaining the scene
save path. Import accepts BG2/VWGLB, OBJ/MTL, static glTF/GLB and JSON prefabs.

New Scene and Close Scene create an empty camera-bearing document. Replacement
and quit use Save/Discard/Cancel protection. Save/Save As use the Composer's
Name/Name.json layout and complete draw resource writer, including GPU-only HDR
lightmaps. Export Selected Model(s) writes only directly selected models and
joints, excluding children and node placement. Multiple selection writes separate
Name/Name.bg2 directories; partial batch failure retains prior completed exports.

The inherited camera/material/selection/UV/bake controls remain the UI foundation.
Scene-tree/component/controller UI and asynchronous HTTP/local imports are now
integrated. Scene-wide multiple bake jobs remain pending. See the
[IO/clone handoff](../../../docs/render_to_draw/plan/milestone_07_bg2e_composer/scene-io-cloning.md)
and [lead regression checks](../../../tests/draw/composer_io/README.md).


The Scene hierarchy supports queued selection, local-transform-preserving drag
reparenting, child creation, enabled state and a staged reflected component
inspector. Ctrl-click extends hierarchy selection; separate submesh rows select
material targets. Structural edits address the primary selected node. The scene
retains at least one enabled camera and at most eight enabled authored lights;
the stage fill is suppressed at eight. Only the active camera's authored orbit
controller receives viewport input. Static cameras render without a controller.
See the [M07-03 handoff and validation checklist](../../../docs/render_to_draw/plan/milestone_07_bg2e_composer/scene-editing.md).


Window > Import Settings controls the localhost service (default port 8643,
enabled; independent draw.bg2e_composer.import preferences) and local source
units/Z-up conversion. Imports use a joined CPU reader and stopped main-thread
GPU realization; the loading panel supports cancel and up to 16 waiting local
jobs. HTTP POST /import accepts glTF/GLB and preserves canonical-path reimport;
GET /status remains available while a request is busy. Only one HTTP request is
admitted at a time. A bind error is reported if the legacy Composer owns the same
port. See [lifecycle/contracts](../../../docs/render_to_draw/plan/milestone_07_bg2e_composer/async-import-service.md)
and [lead checks](../../../tests/draw/composer_import/README.md). GPU realization
is synchronous; cancellation prevents late publication but cannot interrupt that
call or a third-party CPU parser mid-read.
