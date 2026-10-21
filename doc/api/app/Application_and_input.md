# Application and Input

**Headers:** `<bg2e/app/Application.hpp>`, `<bg2e/app/InputDelegate.hpp>`,
`<bg2e/app/InputManager.hpp>`, `<bg2e/app/KeyEvent.hpp>`  
**Namespace:** `bg2e::app`

## Application

`Application` is the interactive application configuration object passed to
`MainLoop::run()`.

```cpp
class Application {
public:
    virtual void init(int argc, char** argv) = 0;

    void setRenderDelegate(std::shared_ptr<draw::RenderLoopDelegate>);
    void setInputDelegate(std::shared_ptr<InputDelegate>);
    void setUiDelegate(std::shared_ptr<ui::UserInterfaceDelegate>);
};
```

Call all three setters from the derived application's `init()` implementation.
A common delegate may implement all three interfaces, or each role may have a
separate owner.

## Graphics delegate registration

Application stores one draw graphics delegate. `setRenderDelegate(nullptr)` clears
that slot through the shared-pointer parameter. Input/UI registration is independent.

```cpp
std::shared_ptr<draw::RenderLoopDelegate>& renderDelegate();
std::shared_ptr<InputDelegate>& inputDelegate();
std::shared_ptr<ui::UserInterfaceDelegate>& uiDelegate();
```

The getters return mutable references; prefer the setters for registration.
`MainLoop::run(application)` uses draw with platform defaults. Pass an explicit
`draw::EngineConfig` to override the backend or initial formats. Both overloads
require non-null graphics, UI and input delegates and validate before window/GPU
allocation. Call the application's `init(argc, argv)` before `run()`.

See [draw::RenderLoopDelegate](../draw/RenderLoopDelegate.md) for callbacks without
Vulkan frame resources or a central descriptor allocation callback. Implement
`UserInterfaceDelegate::init(draw::Engine*, UserInterface*)` for UI initialization
and use engine UI wrappers. The remaining legacy shared scene/UI resource
overloads are scheduled for cleanup; they do not select another application path.

## InputDelegate

Override only the events the application needs:

```cpp
class EditorInput : public InputDelegate {
public:
    void keyDown(const KeyEvent& event) override;
    void keyUp(const KeyEvent& event) override;
    void mouseMove(int x, int y) override;
    void mouseButtonDown(int button, int x, int y) override;
    void mouseButtonUp(int button, int x, int y) override;
    void mouseWheel(int deltaX, int deltaY) override;
    void fileDropped(const std::filesystem::path& path) override;
};
```

All methods have empty defaults. Mouse button indexes are `0` for left, `1`
for middle, and `2` for other/right buttons in the current main-loop mapping.

`InputDelegate` is the application event boundary; it does not automatically
dispatch events to scene components. Applications that use scene controllers
must forward uncaptured input through `scene::InputVisitor` (or their own
component event routing). The manipulation sample combines that routing with
`SelectionManager`: uncaptured mouse gestures reach `OrbitCameraController`,
while captured gizmo gestures remain with the selection manager. See the
[manipulation tutorial](../../tutorials/draw/11_manipulation.md#14-give-input-its-own-small-state-machine).

## KeyEvent

`KeyEvent` wraps the engine key enum. `key()` returns the key,
`isModifier()` identifies Shift/Control/Alt/Super keys, and `keyName()` returns
a display name. `fromSDLEvent()` performs the SDL conversion used by
`MainLoop`.

## InputManager, Keyboard, and Mouse

`InputManager` forwards translated events to its delegate and provides
`normalizedCursorPosition(width, height)`. `Keyboard` and `Mouse` expose
current device state for polling-style input. Applications normally receive
events through `InputDelegate`; `MainLoop` owns its `InputManager`.

## Event cadence under background limiting

SDL events continue to be polled while complete frames are throttled. Input
callbacks may therefore run between rendered background frames. Visual changes
caused by those callbacks become visible on the next scheduled or explicitly
requested frame. Call `MainLoop::current()->requestFrame()` when an action must
produce a prompt background frame.

