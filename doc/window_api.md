# Window API Documentation

## 1. Overview
The window system of the engine provides a unified and backend-agnostic way to define and create the main application window.
All configuration is expressed through a single structure: **`WindowConfig`**, and the window is created using:

```cpp
void MainLoop::initWindow(const WindowConfig& config);
```

The last call to `initWindow()` takes precedence and replaces any previous configuration.

## 2. `WindowConfig`

`WindowConfig` represents all properties of the application window at creation time.
It does not expose SDL or any external API in the public interface.

### Definition

```cpp
struct WindowConfig {
    std::string title = "";

    int32_t x = -1;
    int32_t y = -1;

    uint32_t width = 1280;
    uint32_t height = 720;

    bool isMaximized = false;
    bool isFullscreen = false;

    bool resizable = true;
    bool decorated = true;
    bool visible = true;
    bool alwaysOnTop = false;

    static WindowConfig withSize(const std::string& title,
                                 uint32_t w, uint32_t h);

    static WindowConfig withPositionAndSize(const std::string& title,
                                            int32_t x, int32_t y,
                                            uint32_t w, uint32_t h);

    static WindowConfig maximized(const std::string& title);

    static WindowConfig fullscreen(const std::string& title);
};
```

## 3. Configuration Options

### 3.1 Window size
```cpp
auto cfg = WindowConfig::withSize("My Application", 1600, 900);
```

### 3.2 Position + size
```cpp
auto cfg = WindowConfig::withPositionAndSize("My Application",
                                             100, 50, 1280, 720);
```

### 3.3 Maximized window
```cpp
auto cfg = WindowConfig::maximized("My Application");
```

### 3.4 Fullscreen window
```cpp
auto cfg = WindowConfig::fullscreen("My Application");
```

### 3.5 Additional optional properties
```cpp
auto cfg = WindowConfig::withSize("My Application", 1280, 720);
cfg.resizable = false;
cfg.decorated = false;
cfg.alwaysOnTop = true;
```

## 4. Usage with `MainLoop`

Basic initialization flow:
1. Create a `MainLoop` instance.
2. Call `initWindow()` with a `WindowConfig`.
3. Create the application.
4. Start execution with `run()`.

## 5. Full Example (no delegates)

```cpp
#include <bg2e.hpp>

class MyApp : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override {
        // No delegates → black window
    }
};

int main(int argc, char** argv) {
    using namespace bg2e::app;

    MainLoop mainLoop("org.bg2e.example");

    auto winCfg = WindowConfig::withSize("BG2E Example", 1600, 900);
    mainLoop.initWindow(winCfg);

    MyApp app;
    app.init(argc, argv);

    return mainLoop.run(&app);
}
```

## 6. Notes on Behavior

- `isMaximized = true` overrides any initial width/height.
- `isFullscreen = true` makes the window occupy the entire desktop space.
- The last call to `initWindow()` is authoritative.
