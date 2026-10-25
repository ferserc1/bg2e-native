# Tutorial: Move reflected components into an application plugin

This tutorial starts from the existing [reflection application](../refection/01_scene_and_manipulation.md)
and moves its two custom components into a native plugin. The application keeps the
same scene, camera controls, picking, multi-selection, gizmos, materials, lighting
controls and graphics settings. It keeps the engine NodeEditor window and its
ComponentInspector, including Add Component and reflected properties/actions.

The application-owned manual reflection inspector is omitted. That window explained
how to build a UI from reflection; the engine inspector already provides the UI
needed here. This tutorial focuses on component ownership across a plugin boundary
and automatic discovery rather than reconstructing that manual editor.

**Application target:** `tutorial_plugins`

**Plugin target:** `tutorial_components_plugin`

**Sources:** [example directory](../../../examples/tutorials/plugins/README.md),
[host](../../../examples/tutorials/plugins/src/main.cpp),
[plugin](../../../examples/tutorials/plugins/plugin/TutorialPlugin.cpp),
[CMake](../../../examples/tutorials/plugins/CMakeLists.txt).

## 1. Reuse the reflection tutorial's scene and component behavior

Use the earlier tutorial for the parts that remain the same:

- [Scene and manipulation](../refection/01_scene_and_manipulation.md) explains the scene, controls and application delegates.
- [Rotation component](../refection/02_rotation_component.md) explains its axis, degrees-per-second speed and toggle action.
- [Reflection editor](../refection/03_reflection_editor.md) explains metadata, nested objects and polymorphic samples. Its application-owned editor implementation is not copied here.

The documentation directory is currently spelled `refection`; the links above
follow the actual repository paths.

Rotation remains attached to the original model initially. Reflection Samples
remains optional through Add Component, with scalar/read-only values, nested objects,
resources, polymorphic modes and reset action. Both components keep the original
`ignoreSerialization(true)` policy: this example does not add scene-file serialization.
Moving a class into a plugin does not automatically implement serialization for it.

## 2. Separate host and module sources

The example is organized as follows:

```text
examples/tutorials/plugins/
  CMakeLists.txt
  README.md
  src/
    main.cpp
  plugin/
    TutorialPlugin.cpp
    PluginComponents.hpp
    RotationComponent.hpp
    RotationComponent.cpp
    ReflectionSamplesComponent.hpp
    ReflectionSamplesComponent.cpp
```

`src/main.cpp` contains the host application and original scene/UI/manipulation
code. The app helper collects `src/*.cpp` into the executable. Plugin implementation
files are outside that directory and are listed only in the module target.

Keep `RotationComponent.hpp` and `ReflectionSamplesComponent.hpp` private to the
plugin. The host must not include those headers, compile the definitions into its
own target, construct their concrete classes directly or call their registration
functions. It will use persistent IDs and the engine's shared registry instead.
`PluginComponents.hpp` is also private: it declares the module's two registration
helpers, not a second application-facing ABI.

## 3. Move reflection registration out of static helpers

Keep the component classes and their behavior, including their independent clone
implementations, in the private headers. Their persistent IDs remain:

| Component | ID |
|-----------|----|
| RotationComponent | `tutorial.Rotation` |
| ReflectionSamplesComponent | `tutorial.ReflectionSamples` |

Replace the reflection tutorial's one-time registration helpers with functions
accepting `plugin::ComponentRegistrar&`. Build ordinary reflection descriptions,
but submit the component factory and metadata through **one registrar operation**:

```cpp
void registerRotation(bg2e::plugin::ComponentRegistrar& registrar)
{
    bg2e::reflection::TypeInfoBuilder<RotationComponent> type(RotationComponent::staticTypeName());
    type.displayName("Rotation (tutorial)");
    type.property("axis", &RotationComponent::axis, &RotationComponent::setAxis)
        .displayName("Axis").category("Rotation").combo()
        .enumValue("X", RotationComponent::Axis::X)
        .enumValue("Y", RotationComponent::Axis::Y)
        .enumValue("Z", RotationComponent::Axis::Z)
        .tooltip("Rotate around this local axis");
    type.property("speed", &RotationComponent::speed, &RotationComponent::setSpeed)
        .displayName("Speed (degrees/second)").category("Rotation")
        .drag().range(-360.0, 360.0).step(1.0)
        .tooltip("Negative values reverse rotation; zero pauses it");
    type.action("toggleRotation", &RotationComponent::toggleRotation)
        .displayName("Toggle rotation").category("Rotation")
        .tooltip("Pause or resume rotation without changing the speed or transform");
    registrar.registerComponent<RotationComponent>(type.build());
}
```

Put this function inside `namespace tutorial` in `plugin/RotationComponent.cpp`,
with its private component header, `PluginComponents.hpp` and
`<bg2e/reflection/Builder.hpp>`. It defines metadata
without constructing a scene instance or initializing a GPU. The default component
factory is invoked later when the host or inspector requests an instance.

In `ReflectionSamplesComponent.cpp`, move the supporting definitions through the
same owner-aware registrar before registering the component:

```cpp
registrar.registerReflection(values.build());
registrar.registerReflection(base.build());
registrar.registerReflection(smooth.build());
registrar.registerReflection(step.build());
registrar.registerSubtype<SampleMode, SmoothMode>(
    "tutorial.SampleMode", "smooth", "Smooth", "tutorial.SmoothMode");
registrar.registerSubtype<SampleMode, StepMode>(
    "tutorial.SampleMode", "step", "Step", "tutorial.StepMode");

// component is the TypeInfoBuilder<ReflectionSamplesComponent> with the
// original object properties, polymorphic mode properties and reset action.
registrar.registerComponent<ReflectionSamplesComponent>(component.build());
```

The builders retain the original reflected type names `tutorial.SampleValues`,
`tutorial.SampleMode`, `tutorial.SmoothMode` and `tutorial.StepMode`. The mode classes
remain polymorphic and default-constructible. The property definitions are unchanged;
see the [complete implementation](../../../examples/tutorials/plugins/plugin/ReflectionSamplesComponent.cpp).

Do not retain `TypeRegistration`, `RegisterComponent`, registration statics or direct
singleton writes from the original helper implementations. The registrar gives the
manager ownership of successful factories, supporting metadata and subtype callbacks,
so it can retire them before closing the library. Duplicate attempts preserve the
existing owner and are omitted from this plugin's successful component ID list.

## 4. Implement the Plugin class and declaration macro

The plugin class identifies the module and coordinates the two private registration
helpers. The complete implementation, with the repository license header omitted
here for readability, is:

```cpp
#include <bg2e/plugin/Plugin.hpp>

#include "PluginComponents.hpp"

namespace tutorial {

class TutorialPlugin final : public bg2e::plugin::Plugin {
public:
    bg2e::plugin::PluginInfo info() const override
    {
        bg2e::plugin::PluginInfo result;
        result.id = "org.bg2e.tutorial.plugins.components";
        result.name = "Reflection tutorial components";
        result.description = "Rotation and reflection samples supplied by an automatically discovered plugin";
        result.version = "1.0.0";
        result.provider = "bg2 engine";
        result.licenseExpression = "GPL-3.0-or-later";
        result.licenseUrl = "https://www.gnu.org/licenses/gpl-3.0.html";
        result.metadata.emplace("tutorial", "plugins");
        return result;
    }

    void registerComponents(bg2e::plugin::ComponentRegistrar& registrar,
        const bg2e::plugin::PluginLoadContext&) override
    {
        registerRotation(registrar);
        registerSamples(registrar);
    }
};

}

BG2E_DEFINE_PLUGIN(tutorial::TutorialPlugin)
```

`PluginInfo::version` describes this plugin. Its stable ID, name, description,
provider, extensible metadata and license information belong to the author.
The SDK/ABI versions are a separate concern: `BG2E_DEFINE_PLUGIN` generates the
query/factory/destruction exports and reads compatibility values from the paired
SDK headers/compiler environment. The author does not implement `getPluginApi`,
copy ABI strings or write the plugin's destruction export manually.

Use the macro once in this module implementation file. Construction and registration
happen when the host loads the plugin; no component factory/reflection registration
runs in a global constructor. `registerComponents` must remain graphics-independent
because MainLoop invokes it before creating the graphics execution.

## 5. Attach the module to the app with the shared CMake helper

The example's CMake file is:

```cmake
cmake_minimum_required(VERSION 3.23)

set(APP_TARGET_NAME tutorial_plugins)
if(APPLE)
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
else()
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME}
        OUTPUT_DIRECTORY "${PRODUCT_DIR}/examples/tutorial_plugins")
endif()

bg2e_add_plugin(
    TARGET_NAME tutorial_components_plugin
    APP_TARGET ${APP_TARGET_NAME}
    SOURCES
        plugin/TutorialPlugin.cpp
        plugin/RotationComponent.cpp
        plugin/ReflectionSamplesComponent.cpp)
```

The application helper establishes the output directory before creating runtime
and resource deployment targets. Linux and Windows get a private runtime root;
macOS keeps the ordinary app bundle boundary. The module sources are separate from
the app's source glob.

`bg2e_add_plugin` creates a MODULE target, configures engine public/dependency/SDK
include paths and C++20, and links the application's **same shared engine**.
`ENGINE_TARGET` defaults to the repository's `BG2E_LIB_TARGET` here. Windows imports
engine classes and exports only plugin entry points. Its runtime settings come from
the engine target, not a guess based on a Debug/Release directory name.

The helper gives intermediate plugin artifacts their own build-tree directory.
The application depends on a deployment target that depends on the module. That
copy target refreshes on each app-target build, including module-only changes, and
uses CMake's actual platform filename rather than a hard-coded prefix/suffix.

For helper arguments, standalone applications and the binary-paired SDK profile,
see [CMake integration](../../api/plugin/CMake.md).

## 6. Understand why discovery is automatic

The deployment destination and loader path agree:

| Platform | Application and plugin location |
|----------|---------------------------------|
| Linux | App under `bin/linux/examples/tutorial_plugins`; module under that directory's `plugins`. |
| Windows | App under `bin/windows/examples/tutorial_plugins/<configuration>`; module under that directory's `plugins`. |
| macOS | App's executable under `Contents/MacOS`; module under `Contents/Resources/plugins`. |

`PlatformTools::pluginPath()` returns that resource directory independently of the
working directory. MainLoop installs its session cleanup guard, scans the directory,
validates each module's descriptor and invokes `registerComponents` before graphics
initialization and scene creation. A missing default directory is empty success;
individual incompatible/broken candidates have recoverable diagnostics.

The shared engine is beside the executable on Linux/Windows and in `Contents/lib`
on macOS. Module search paths/import-library linkage resolve that engine; the
helper does not place another engine inside `plugins`. The engine's registry
singletons are therefore shared by the module and application.

This example relies entirely on that automatic discovery. It has no alternative
loading path to hide a deployment error. The host logs discovered plugin information
and committed component IDs during scene creation. A raw `getMessage` library from
the separate dynamic-library example would be classified `NotAPlugin`; it does not
satisfy this tutorial's module contract.

## 7. Create Rotation by its registered ID

The original scene directly constructed RotationComponent. The host now uses:

```cpp
auto* rotation = scene::ComponentFactoryRegistry::get().createDefault("tutorial.Rotation");
if (!rotation)
{
    throw std::runtime_error("The tutorial component plugin was not discovered in plugins before scene creation");
}
rotatingModel->addComponent(rotation);
```

This stays inside `SceneDelegate::createScene`, after MainLoop discovery. It must
not move into `Application::init`, which is called before MainLoop::run and before
automatic registration. The explicit failure makes a missing module/registration
visible instead of silently replacing it with a host-linked implementation.

The application uses only the stable string key and the scene Component interface.
The engine inspector already intersects registered factories with reflection metadata,
so Rotation and Reflection Samples participate in its existing component list,
property editors and actions. No manual component enumeration or new inspector
window is needed. The original engine NodeEditor still follows the first selected
node during multiple selection; graphics/material controls remain unchanged.

## 8. Release scene and UI references before automatic shutdown

The module defines the component methods, destructors, clone control blocks and
reflection callbacks. Those uses must be gone before native code is released.
The application's stopped-boundary hook is:

```cpp
void preparePluginShutdown() override
{
    auto delegate = std::static_pointer_cast<SceneDelegate>(renderDelegate());
    auto ui = std::static_pointer_cast<UiDelegate>(uiDelegate());
    // Drop inspector/selection caches while the plugin and graphics services
    // are still available. All tutorial plugin instances live under this root.
    ui->cleanup();
    if (auto selection = delegate->selection())
    {
        selection->deselect();
    }
    auto root = delegate->scene();
    auto& manager = plugin::PluginManager::get();
    for (const auto id : manager.plugins())
    {
        plugin::Plugin::removeComponents(id, root.get());
    }
}
```

The UI cleanup clears the engine inspector's node, weak selection order and
material/settings bindings; it is idempotent because normal delegate cleanup also
uses it. Selection is cleared before component removal. Each loaded plugin's helper
uses only its successfully registered IDs, includes disabled nodes and removes
matches through normal Node callbacks after traversal.

All plugin instances in this example's live scene are under the one root. Delegate
cleanup then releases queued updates/scene resources while the module is still
mapped. MainLoop retires the registries, cleans the graphics execution, finalizes
plugins and releases their native library references in reverse order.

When extending the application, include every additional active root and release
detached objects, clones, history/clipboard references, weak control blocks and
copied executable callbacks too. Scene removal alone is not proof of destruction.
The programmer owns this contract; an omitted reference can crash after unload.
See the [full unload contract](../../api/plugin/unloading.md) for those obligations
and the difference between scene cleanup and known module leases.

## Related API

- [Plugin](../../api/plugin/Plugin.md) and [ComponentRegistrar](../../api/plugin/ComponentRegistrar.md).
- [SDK/ABI compatibility](../../api/plugin/Sdk.md) and [lifecycle](../../api/plugin/lifecycle.md).
- Explicit plugin loading is also available; it is documented only in the [API loading guide](../../api/plugin/PluginManager.md#explicit-file-and-directory-loading).
