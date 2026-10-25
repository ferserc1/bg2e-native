# Plugin and dynamic-library tutorials

| Tutorial | Example | Focus |
|----------|---------|-------|
| [Automatic application plugins](plugin.md) | `examples/tutorials/plugins` | Move Rotation and Reflection Samples out of the reflection tutorial's host, use the registrar/CMake helper and retain the engine inspector. |
| [Create and load a dynamic library](dynamic_library.md) | `examples/plugins/dynamic_library` | Independent raw `getMessage` export, explicit DynamicLibrary ownership and symbol lookup. |

For the component-plugin tutorial, start with the existing
[reflection scene](../refection/01_scene_and_manipulation.md) and
[component behavior](../refection/02_rotation_component.md). The directory is
spelled `refection` in the current repository.

The [Plugin API](../../api/plugin/index.md) documents explicit plugin loading,
SDK/ABI matching, lifecycle callbacks and the mandatory programmer-owned unload
contract. The automatic tutorial mentions explicit loading only through that API
reference; its implementation relies on MainLoop discovery.
