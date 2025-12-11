# BG2E Delegate System Documentation

## Overview

The BG2E engine uses a **delegate-based architecture** that separates application logic into three independent functional domains:

- **Render Delegate** (`bg2e::render::RenderLoopDelegate`)
- **Input Delegate** (`bg2e::app::InputDelegate`)
- **UI Delegate** (`bg2e::ui::UserInterfaceDelegate`)

An application assigns delegates during initialization, and the engine routes events to them throughout the execution loop.  
Delegates may be implemented in a single class or split across multiple classes.

---

## 1. Assigning Delegates to an Application

Applications subclass `bg2e::app::Application` and assign delegates inside `init()`:

```cpp
class MyApplication : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override {
        auto delegate = std::make_shared<AppDelegate>();
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(delegate);
    }
};
```

Delegates may be:
- A **single class** implementing all three interfaces (common for editors)
- Several independent classes

---

## 2. Combined Delegate Example (ModelEdit)

```cpp
class AppDelegate :
    public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    // Implements rendering, input, and UI behavior
};
```

This class handles:
- Rendering through a default render loop delegate
- User input events
- ImGui UI drawing and layout

The engine allows complete freedom to implement delegates in any combination.

---

## 3. Render Delegates

### Base Class: `RenderLoopDelegate`

The base class for render delegates is:

```cpp
bg2e::render::RenderLoopDelegate
```

Users may subclass it and implement the entire rendering pipeline manually.

### Optional Convenience Class: `DefaultRenderLoopDelegate<T>`

For simpler implementations, BG2E provides:

```cpp
bg2e::render::DefaultRenderLoopDelegate<RendererType>
```

This template:

- Owns a renderer of type `RendererType`
- Automatically calls the renderer’s lifecycle methods:
  - `init()`
  - `beginFrame()`
  - `draw()`
  - `endFrame()`
- Reduces boilerplate significantly

It is **not mandatory**—users can directly subclass `RenderLoopDelegate` if they prefer full control.

---

## 4. Input Delegate

The **input delegate** receives events such as keyboard, mouse, and file drops.

```cpp
class InputDelegate {
public:
    virtual void keyDown(const KeyEvent& key) {}
    virtual void keyUp(const KeyEvent& key) {}
    virtual void mouseMove(int x, int y) {}
    virtual void mouseButtonDown(int button, int x, int y) {}
    virtual void mouseButtonUp(int button, int x, int y) {}
    virtual void mouseWheel(int dx, int dy) {}
    virtual void fileDropped(const std::filesystem::path& path) {}
};
```

SDL events are forwarded to this delegate by the internal `MainLoop`.

---

## 5. UI Delegate

The **UI delegate** encapsulates all ImGui-based UI operations.

Typical responsibilities:
- Initializing UI systems
- Drawing user interface each frame
- Managing workspace layouts (`bg2e::ui::Workspace`)

```cpp
class UserInterfaceDelegate {
public:
    virtual void initUI() {}
    virtual void drawUI() {}
};
```

### Automatic viewport helpers: `uiWidth()` and `uiHeight()`

`UserInterfaceDelegate` provides **inline helper functions**:

```cpp
inline uint32_t uiWidth() const;
inline uint32_t uiHeight() const;
```

These functions:
- Return the current UI viewport size
- Cannot be overridden
- Eliminate the need for manually tracking window size
- Are updated automatically by the engine each frame

The Workspace layout system is designed to use these helpers for consistent panel sizing.

---

## 6. Why Delegates?

### Separation of Responsibilities

| Domain | Delegate |
|--------|----------|
| Rendering | `RenderLoopDelegate` |
| Input | `InputDelegate` |
| User Interface | `UserInterfaceDelegate` |

### Modular Architecture

You may:
- Replace the renderer by changing the render delegate
- Change UI behavior without touching rendering logic
- Implement input in a separate component

### Low Coupling

Delegates do not depend on each other; the `Application` coordinates them.

---

## 7. Frame Execution Flow

1. `MainLoop` polls SDL events  
2. Input events are forwarded to `InputDelegate`  
3. Render loop begins (`RenderLoopDelegate::beginFrame()`)  
4. Renderer draws scene  
5. UI delegate draws the UI (`drawUI()`)  
6. Frame ends  
7. Loop repeats  

This ordering ensures stable integration of rendering and UI.

---

## 8. Example of a Fully Integrated AppDelegate

```cpp
class AppDelegate :
    public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    void keyDown(const KeyEvent& key) override { /* ... */ }
    void drawUI() override { /* ImGui code ... */ }
};
```

---

## Summary

The BG2E delegate system provides:

- A clean, modular architecture separating rendering, UI, and input
- Flexible configuration: single or multiple delegate classes
- Optional convenience classes like `DefaultRenderLoopDelegate`
- Easy UI management through `UserInterfaceDelegate` and helpers like `uiWidth()` and `uiHeight()`
- A predictable and clean execution flow managed by `MainLoop`

It is a powerful structure for building tools, editors, visualization software, and any application requiring clear separation of concerns.

