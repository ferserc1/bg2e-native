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
    void setInputDelegate(std::shared_ptr<InputDelegate>);
    void setUiDelegate(std::shared_ptr<ui::UserInterfaceDelegate>);
};
```

Call the three setters from the derived application's `init()` implementation.
A common application delegate implements all three interfaces and is shared by
the corresponding setters.

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

