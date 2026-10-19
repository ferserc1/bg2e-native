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

The initial editable `Scene` contains a cube; the orbit camera, floor and fill
light are stage fixtures outside that document. The document can be edited and
saved with the complete draw resource writer. Open/glTF/OBJ currently replace the
document, with Save/Discard/Cancel protection. They do not insert imported nodes.
The inherited camera/material/selection/UV/bake controls are available as a
baseline; scene-aware camera adoption and multi-job baking remain pending.

See the [feature inventory](../../../docs/render_to_draw/plan/milestone_07_bg2e_composer/application-inventory.md)
for exact gaps and lead checks. M07-02 adds Composer graph IO/import/cloning
semantics; M07-03 adds its tree/components/manipulation UI; M07-04 adds guarded
asynchronous/HTTP import; M07-05 adds scene-wide bake orchestration.
