# Draw Composer baseline

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
Full Composer tree/component/controller UI, fixture/light policy, HTTP imports and
multiple bake jobs are still pending. See the
[IO/clone handoff](../../../docs/render_to_draw/plan/milestone_07_bg2e_composer/scene-io-cloning.md)
and [lead regression checks](../../../tests/draw/composer_io/README.md).
