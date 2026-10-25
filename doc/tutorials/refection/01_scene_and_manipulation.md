# Part 1 — A selectable scene and the built-in node editor

[Next: Create and register a rotating component](02_rotation_component.md)

This tutorial builds an application that lets you select objects in a 3D scene,
inspect their components, add a rotating component and edit its axis and speed.
You will then build a second node editor from reflection metadata. Both editors
will operate on the same node, so a change in either window is visible in the
other.

The only programming prerequisite is C++ (including classes, lambdas and smart
pointers). Basic theoretical knowledge of 3D graphics is sufficient. Engine
concepts are introduced below. The files use C++20, like the engine.

The final source lives in
[`examples/tutorials/reflection`](../../../examples/tutorials/reflection/).
The documentation directory is deliberately named `refection`, matching this
tutorial's requested location; the application's directory is `reflection`.
Follow these three pages in order:

1. **This page:** scene, selection and `ui::NodeEditor`.
2. [Rotation component](02_rotation_component.md): behavior, metadata and factories.
3. [An editor built from reflection](03_reflection_editor.md): discovery, widgets,
   safe edits, nested objects, polymorphism and a complete property playground.

## 1. Understand the objects we will assemble

A bg2 engine scene is a tree of `scene::Node` objects. Each node has a name, an
enabled flag, a parent and children. Children inherit their ancestors' spatial
transforms. A node does not automatically contain geometry, a camera or a light.
Those capabilities come from **components** attached to it:

| Component | Role in this application |
| --- | --- |
| `scene::TransformComponent` | Stores the node's local transformation. |
| `scene::DrawableComponent` | Associates renderable geometry and materials with a node. |
| `scene::CameraComponent` | Holds a projection; its node's transform defines the camera pose. |
| `scene::OrbitCameraComponent` | Changes that pose in response to mouse input. |
| `scene::LightComponent` | Supplies a light. |
| `scene::EnvironmentComponent` | Supplies the environment used for background and lighting. |
| `manipulation::SelectableComponent` | Makes a node participate in selection. |
| `manipulation::GizmoComponent` | Supplies editor handles for manipulation. |

Nodes own their components. Pass `new ComponentType()` to `addComponent()` only
when transferring ownership to the node; never delete that pointer afterwards.
The API also accepts `shared_ptr` and `unique_ptr`. The tree uses shared ownership
for children; a component's `ownerNode()` is a borrowed pointer back to its node.

`draw::Drawable` is the geometry/material resource held by a drawable component.
It is not itself a scene node. `scene::Scene` maintains scene-level collections,
such as lights and cameras, derived from a root node. The renderer consumes the
scene through `draw::DefaultRenderLoopDelegate`.

The example uses the existing `bg2e::draw` renderer and its GPU backend. No engine
API or alternative rendering path needs to change for this tutorial.

## 2. Start from the manipulation application

The starting point is the existing
[`11_manipulation` application](../../../examples/draw/11_manipulation/src/main.cpp).
It already handles the renderer, assets, selection, material editing, camera
input and resource cleanup. Using its source as our initial file avoids hiding
those prerequisites behind an unexplained application framework.

If you want to build that foundation from an empty file first, follow
[the manipulation tutorial](../draw/11_manipulation.md), then return here.
For this tutorial, copy its completed source. From the repository root:

```sh
mkdir -p examples/tutorials/reflection/src
cp examples/draw/11_manipulation/src/main.cpp examples/tutorials/reflection/src/main.cpp
```

When working in a checkout that already includes the final tutorial, do not
replace its source unless you intend to reconstruct it. Use a separate working
copy for the step-by-step exercise.

Keep the full GPL comment at the start of every new C++ file. Copy it from the
original `main.cpp`, before any `#pragma once` or `#include`.

Here is how the copied program works; these relationships matter when we add UI:

- `Application::init()` constructs three delegates and passes them to the
  application: rendering/scene management, input handling and user interface.
- `SceneDelegate` derives from `draw::DefaultRenderLoopDelegate`. Its renderer
  configuration includes a deferred renderer, scaled output, lighting effects
  and the gizmo/selection overlay. Continuous rendering is enabled, so animation
  keeps advancing without user input.
- `SceneDelegate::init()` calls the base implementation and initializes a
  `manipulation::SelectionManager`. It enables multiple selection and uses the
  same logical picking scale as the source example.
- `createScene()` builds a root node, a group of models, an independently cloned
  model, transparent objects, a floor and texture experiments. `addDrawable()`
  creates a node with a transform and a drawable component, then adds selection
  and gizmo components for selectable objects. The camera and light group are
  separate branches. `_state.sceneContext` points to a `scene::Scene` built from
  that root.
- `ManipulationInput` distinguishes clicks from drags, lets the selection manager
  capture gizmo gestures and forwards remaining gestures to the scene camera
  through `scene::InputVisitor`. Keep the balanced mouse-down/mouse-up forwarding:
  otherwise a camera controller can remain in dragging mode.
- `UiDelegate` owns the controls window, graphics window and material editor.
  Its `init()` receives the live engine/UI after rendering initialization.
  `drawUI()` draws widgets each frame; widget functions return whether their
  temporary value changed. `cleanup()` releases editor bindings before graphics
  resources disappear.
- `main()` creates `app::MainLoop`, initializes the application and runs it. The
  copied cleanup code releases drawables, gizmos and the viewpoint analyzer.
  Preserve it even though reflection itself does not allocate GPU resources.

Assets are located with `base::PlatformTools::assetPath()`, not relative to your
shell's working directory. The build helper deploys the existing engine shaders
and assets, including the OBJ model, JPEG maps and HDR environment.

## 3. Give the application its own build target and identity

Create `examples/tutorials/reflection/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME tutorial_reflection)
bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
```

Add this line to `examples/CMakeLists.txt`:

```cmake
add_subdirectory(tutorials/reflection)
```

These entries are already present in the finished checkout. Do not add them
twice. `bundle_app_sdl` is the same helper used by `11_manipulation`: it collects
`src/*.cpp`, provides include paths, links the engine and SDL, sets output paths
and packages the runtime resources. New example directories are not discovered
by that helper until their `CMakeLists.txt` is included with `add_subdirectory`.

In the copied `main.cpp`, replace the MainLoop identifier with
`org.bg2e.examples.tutorial.reflection`, the window title with
`bg2e tutorial: reflection`, and the exception prefix with
`tutorial_reflection: `. These distinguish the application's window and stored
preferences. In `UiDelegate`, replace the preferences key
`draw.manipulation.graphics` with `tutorial.reflection.graphics`.
Change the root node's name in `createScene()` to `Reflection tutorial scene`.

If your checkout has already been configured, regenerate it using the same
platform generator and Vulkan SDK. For a new build directory, choose the
appropriate example below; replace the SDK path with your installed SDK root:

```sh
# Linux
cmake -S . -B build-reflection -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk -DBG2E_BUILD_EXAMPLES=ON

# macOS: Xcode is required for working native file dialogs
cmake -S . -B build-reflection -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk -DBG2E_BUILD_EXAMPLES=ON

# Windows (PowerShell)
cmake -S . -B build-reflection -G "Visual Studio 17 2022" -DVULKAN_SDK=C:/VulkanSDK/your-version -DBG2E_BUILD_EXAMPLES=ON
```

Build the target with:

```sh
cmake --build build-reflection --target tutorial_reflection --config Debug
```

`--config` selects the configuration for Xcode/Visual Studio; Ninja uses the
configuration chosen at configure time. Run `tutorial_reflection` from the
configured product directory under `bin` (on macOS, open its `.app` bundle).
The exact platform/configuration path is controlled by the repository's CMake
product settings. Dependencies and the Vulkan SDK must already be installed,
just as for the other examples. These are instructions for the reader; the
tutorial sources do not require a new shader or an engine rebuild procedure.

## 4. Add the built-in node editor window

Add to the includes in `main.cpp`:

```cpp
#include <bg2e/ui/NodeEditor.hpp>
```

In `UiDelegate`'s private data, after `_controls`, add:

```cpp
ui::Window _nodeWindow;
ui::NodeEditor _nodeEditor;
std::vector<std::weak_ptr<scene::Node>> _selectionOrder;
```

The window supplies a title, position and drawable region. `NodeEditor` draws
its contents; it does not own a top-level window or the selected node. Weak
references in `_selectionOrder` remember selection order without retaining
removed scene objects indefinitely.

After `_controls.setTitle("Manipulation")` in the constructor, add:

```cpp
_nodeWindow.setTitle("NodeEditor (engine)");
_nodeWindow.setPosition(10, 320);
_nodeWindow.setSize(330, 460);
```

Replace the inherited `_controls.setSize(430, 700)` with
`_controls.setSize(330, 300)`. This puts controls above the node editor on the left
and leaves the center of the viewport available for picking. The controls window
scrolls when its contents exceed that size. Both windows can be moved/resized.

Move the inherited graphics window to `(200, 100)` and replace its constructor's
`_graphics.open()` with `_graphics.close()`. Its existing “Graphics settings”
button still opens it; keeping it closed initially leaves room for our editors.

## 5. Keep the first-selected node during multiple selection

`NodeEditor::setNodes()` deliberately displays a multiple-selection placeholder
when given several nodes. Our requirement is different: edit the first node
that was selected, while leaving the other selections intact. Therefore we call
`setNode()` with exactly one pointer.

`SelectionManager::selectedNode()` is convenient for the inherited manipulation
commands, but the manager can prioritize drawable nodes over node-only selections.
To make our editor's chronological policy explicit, add this private method to
`UiDelegate`:

```cpp
    std::shared_ptr<scene::Node> firstSelectedNode(const manipulation::SelectionManager& selection)
    {
        // Preserve click order independently of SelectionManager's drawable priority.
        const auto& selected = selection.selectedNodes();
        const auto stillSelected = [&](const std::shared_ptr<scene::Node>& node)
        {
            return node && std::any_of(selected.begin(), selected.end(), [&](const auto& weak)
            {
                return weak.lock() == node;
            });
        };
        std::erase_if(_selectionOrder, [&](const auto& weak)
        {
            return !stillSelected(weak.lock());
        });
        for (const auto& weak : selected)
        {
            auto node = weak.lock();
            const bool known = std::any_of(_selectionOrder.begin(), _selectionOrder.end(), [&](const auto& entry)
            {
                return entry.lock() == node;
            });
            if (node && !known)
            {
                _selectionOrder.push_back(node);
            }
        }
        return _selectionOrder.empty() ? nullptr : _selectionOrder.front().lock();
    }
```

Each selection change removes nodes that are no longer selected and appends
newly selected nodes. Existing entries keep their position. If the first node
is deselected, the next surviving entry becomes the target. Selecting two
submeshes of one node still edits one node because `selectedNodes()` is unique
at node level.

In `UiDelegate::init()`, after `_editor.onChanged(...)`, add:

```cpp
const auto weakUi = weak_from_this();
const auto weakSelection = std::weak_ptr<manipulation::SelectionManager>(delegate->selection());
delegate->selection()->onSelect([weakUi, weakSelection]()
{
    auto ui = weakUi.lock();
    auto selection = weakSelection.lock();
    if (ui && selection)
    {
        ui->firstSelectedNode(*selection);
    }
});
```

The copied code already declares `weakUi` for `setUiCleanup()`: reuse that
variable instead of declaring it twice. Calling the helper from the selection
callback records changes even if several input events arrive between UI frames.
`UiDelegate` is created with `make_shared`, so `weak_from_this()` is valid here.
The callback uses weak captures to avoid a reference cycle through the selection
manager.

## 6. Connect edits and draw the window

Also in `UiDelegate::init()`, before the selection callback, add:

```cpp
_nodeEditor.onChanged([weakDelegate]()
{
    if (auto active = weakDelegate.lock())
    {
        active->enqueueRendererUpdate(active->rendererGeneration(), [](draw::Renderer& renderer)
        {
            renderer.invalidateHistories();
        });
    }
});
```

`weakDelegate` is already defined in the copied `init()`. This schedules temporal
history invalidation against the current renderer. It does not defer the built-in
editor's setters: the shipped `NodeEditor` directly edits its node and components.
Part 3 will teach an explicitly queued edit route for the editor we own. Continuous
rendering also makes node name/enabled changes visible, even though those two
controls do not currently emit `NodeEditor::onChanged()`.

Add this private method to `UiDelegate`:

```cpp
void drawNodeWindows(const std::shared_ptr<SceneDelegate>& delegate)
{
    auto node = firstSelectedNode(*delegate->selection());
    _nodeEditor.setNode(node.get());
    _nodeWindow.draw([&]
    {
        ui::Text::text("Editing the first selected node, including during multi-selection.");
        _nodeEditor.draw();
    });
}
```

The local `shared_ptr` keeps the selected node alive throughout the immediate UI
call. Never save the raw pointer for a later queued operation. Call this method
in `drawUI()` after its existing early-return guard and before `_graphics.draw()`:

```cpp
drawNodeWindows(delegate);
```

At the beginning of `UiDelegate::cleanup()`, add:

```cpp
_nodeEditor.setNode(nullptr);
_selectionOrder.clear();
```

This clears borrowed UI pointers before scene destruction.

## 7. Check the first milestone

Run the application and click the original model. The built-in editor shows its
node name, enabled flag and reflected components. Expand Transform and change
Translation; you should see the model move. Camera orbiting and gizmo dragging
continue to use the inherited input path.

Ctrl-click another object. Both remain selected, but the node editor must still
show the first object. Ctrl-click the first object again to remove it from the
selection; the editor switches to the next surviving node. “Deselect” shows
“No selection”. Delete the selected object through the inherited controls and
confirm that the editor no longer points at the removed node.

At this stage our code knows which node is selected, but it does not know how
to create UI for a new component. That is the purpose of reflection metadata.

[Next: Create and register a rotating component](02_rotation_component.md)
