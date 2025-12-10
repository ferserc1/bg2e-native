# Input Delegate System

## Overview

The **InputDelegate** interface defines how applications receive input events from the window system.  
It is part of the `bg2e::app` module and is used together with `Application` and `MainLoop`.

The delegate receives:
- Keyboard events  
- Mouse movement and buttons  
- Mouse wheel  
- File drop events (drag & drop)

A user implements this delegate to customize how the application reacts to input.

---

## `InputDelegate` Interface

```cpp
namespace bg2e {
namespace app {

class InputDelegate {
public:
    virtual void keyDown([[maybe_unused]] const KeyEvent& keyEvent) {}

    virtual void keyUp([[maybe_unused]] const KeyEvent& keyEvent) {}

    virtual void mouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseButtonDown([[maybe_unused]] int button, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseButtonUp([[maybe_unused]] int button, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseWheel([[maybe_unused]] int deltaX, [[maybe_unused]] int deltaY) {}

    virtual void fileDropped([[maybe_unused]] const std::filesystem::path &) {}
};

}
}
```

All methods are optional; applications only override what they need.

---

## File Drop Handling

The input system supports receiving files dropped onto the application window (drag & drop).

The method:

```cpp
virtual void fileDropped(const std::filesystem::path& path);
```

Receives the **absolute path** of the dropped file.  
Backend-specific details (SDL `SDL_DROPFILE` event) are hidden from the public API.

Example typical uses:
- Load a 3D model  
- Import textures or images  
- Open a saved project  
- Import engine assets  

---

## Example: Implementing Input Handling in a Delegate

```cpp
#include <bg2e.hpp>
#include <iostream>

class AppDelegate :
    public bg2e::app::RenderDelegate,
    public bg2e::app::InputDelegate,
    public bg2e::app::UiDelegate
{
public:
    void keyDown(const KeyEvent& key) override {
        std::cout << "Key pressed: " << key.keyCode << std::endl;
    }

    void mouseMove(int x, int y) override {
        std::cout << "Mouse moved: " << x << ", " << y << std::endl;
    }

    void fileDropped(const std::filesystem::path& path) override {
        std::cout << "Dropped file: " << path << std::endl;
    }
};
```

---

## Registering the Delegate

```cpp
class MyApplication : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override {
        auto delegate = std::make_shared<AppDelegate>();

        setRenderDelegate(delegate);
        setInputDelegate(delegate);  // Input events handled here
        setUiDelegate(delegate);
    }
};
```

The `MainLoop` receives backend events and routes them to the active input delegate.

---

## Behavior Summary

- All input callbacks are optional.  
- `fileDropped()` always receives a valid filesystem path.  
- The API is backend-agnostic: SDL details are hidden.  
- Only one input delegate is active at any time.

---

## Full Example

```cpp
int main(int argc, char** argv) {
    bg2e::app::MainLoop loop("org.bg2e.example");
    loop.initWindow(WindowConfig::withSize("Input Example", 1280, 720));

    MyApplication app;
    app.init(argc, argv);

    return loop.run(&app);
}
```

This creates a window capable of responding to:
- keyboard events  
- mouse events  
- file drop events  

All handled through the application's `InputDelegate`.

---
