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
