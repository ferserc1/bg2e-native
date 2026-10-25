# Plugin tutorial application

Application target: `tutorial_plugins`. Module target: `tutorial_components_plugin`.

This application preserves the scene, camera controls, picking, multi-selection,
gizmos, material tools, lighting controls and graphics settings of the
[reflection tutorial application](../reflection/README.md). It retains the engine
NodeEditor window, including its ComponentInspector. The application-owned manual
reflection inspector and TutorialNodeEditor are omitted.

`plugin/` privately implements Rotation and Reflection Samples, including their
supporting value types and polymorphic modes. Rotation is attached to the original
model via its registered `tutorial.Rotation` identifier. Reflection Samples remains
available through the engine inspector's Add Component list. Their serialization
policy is unchanged: these tutorial components are excluded from scene files.
The executable neither includes these implementation headers nor registers types.

`bg2e_add_plugin` deploys the module to the application's private `plugins` folder.
MainLoop discovers it before graphics/scene initialization; the application has no
explicit-load fallback. Linux/Windows use the isolated runtime root
`bin/<platform>/examples/tutorial_plugins` (plus the configuration on Windows).
macOS uses the application's normal bundle and `Contents/Resources/plugins`.

`Application::preparePluginShutdown` clears UI/selection state and removes plugin
components from the scene while the library and graphics services are still
available. The programmer must release every additional external plugin use too;
see the [unload contract](../../../doc/api/plugin/unloading.md).

For scene and component background, start with the existing
[reflection tutorial](../../../doc/tutorials/refection/01_scene_and_manipulation.md).
Continue with the [step-by-step plugin walkthrough](../../../doc/tutorials/plugins/plugin.md).
The reusable CMake helper is described in [cmake/PLUGINS.md](../../../cmake/PLUGINS.md).
