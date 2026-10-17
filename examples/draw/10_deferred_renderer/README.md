# Deferred renderer example

`draw_deferred_renderer` demonstrates the basic structure of a scene rendered
with `DefaultRenderLoopDelegate` and `RendererDeferred`. The example builds a
scene in `SceneDelegate::createScene()`, loads its drawable resources when the
render engine is available, and animates the camera and lights in `update()`.

The scene contains a textured two-submesh model and an independent clone,
transparent model variants, a floor, an HDR environment, four lights, and two
procedural textures. The small UI lets you toggle camera and light animation,
change each light's color and intensity, edit the clone's tint and roughness,
and show or hide the cutout canopy.

Scene changes that affect rendered resources are queued through
`enqueueRendererUpdate()` so they are applied at a renderer update boundary.
`Drawable::cleanup()` releases GPU resources before the engine is destroyed;
the `shared_ptr` members themselves are released by normal object destruction.

The example accepts no command-line options. Configure and build it with the
normal project build, then run `draw_deferred_renderer` from `bin/<platform>`.

## Advanced effect controls

The shared Graphics settings window exposes renderer IBL/AO/GI selection,
independent reflection controls, query shadows, temporal accumulation, spatial
denoise, resolution, optional FSR and optional SMAA. Enable the effect as well as
selecting its indirect-lighting mode. Requested values persist even on unsupported
hardware; inspect read-only availability/status and graph selection separately.
Both FSR and SMAA start disabled, preserving ordinary resampling. SMAA processes
final scene color before overlays/UI and also works without RT.

Pause camera/light animation to inspect temporal convergence. Scene/material and
gizmo edits reset SDK/engine history through the synchronized application boundary.
Use lighting debug modes for resolved ray visibility, diffuse GI, reflected
radiance and fractional reflection confidence. Disable final color processing/use
Raw HDR when examining numeric HDR signals. The last processed layer is shown.

[Integration and capability limits](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/integration.md)
· [Lead validation matrix](../../../tests/draw/advanced_integration/README.md).

### Opening graphics settings

The separate **Graphics settings** window opens beside the scene controls at
startup. If closed, use the **Graphics settings** button at the top of the scene
controls to reopen it. Expand **Renderer** to select **Indirect lighting = GI**,
then **Global illumination > Enabled**. Reflections have an independent
**Reflections > Enabled** switch. Read **Available** and **Status** in each effect
section. Save/restore preferences and JSON import/export use the same live catalog;
no preferences are restored automatically. Each example uses its own preference scope.
