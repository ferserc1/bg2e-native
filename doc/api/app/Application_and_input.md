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

    void setRenderDelegate(std::shared_ptr<render::RenderLoopDelegate>);
    void setRenderDelegate(std::shared_ptr<draw::RenderLoopDelegate>);
    void setRenderDelegate(std::nullptr_t);
    void setInputDelegate(std::shared_ptr<InputDelegate>);
    void setUiDelegate(std::shared_ptr<ui::UserInterfaceDelegate>);
};
```

Call the three setters from the derived application's `init()` implementation.
In production, a common application delegate can implement all three interfaces
and be shared by the corresponding setters.

## Graphics delegate registration

The render and draw delegates are distinct types with distinct contracts.
Register either through `setRenderDelegate()`. Each overload sets its own
slot and clears the other, so the last registration wins. Passing `nullptr`
clears both graphics slots, including when a typed empty shared pointer is used.
Input/UI registration is independent.

```cpp
std::shared_ptr<render::RenderLoopDelegate>& renderDelegate();
std::shared_ptr<draw::RenderLoopDelegate>& drawDelegate();
std::shared_ptr<InputDelegate>& inputDelegate();
std::shared_ptr<ui::UserInterfaceDelegate>& uiDelegate();
```

The getters return mutable references. Direct assignment can bypass the setter
invariant and populate both graphics slots; MainLoop rejects that configuration.
Prefer setters. The getter return type cannot select a run overload for you.

Use `run(application)` with a production delegate and
`run(application, draw::EngineConfig{})` with a draw delegate. Both
require non-null input and UI delegates. A valid draw run initializes the selected
GPU backend, window, scene coordinator and UI after configuration validation.

See [draw::RenderLoopDelegate](../draw/RenderLoopDelegate.md) for the experimental
contract. It does not expose Vulkan frame resources or a central descriptor
allocation callback. UserInterfaceDelegate has separate production and draw
initialization overloads; implement `init(draw::Engine*, UserInterface*)` for the
draw path and draw widgets through UI wrappers. Vulkan/Metal interoperability
is private to ui. Widgets using render scene/resources require compatible render
objects.
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

