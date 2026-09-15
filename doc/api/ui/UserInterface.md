# UserInterface and UserInterfaceDelegate

**Header:** `<bg2e/ui/UserInterface.hpp>`, `<bg2e/ui/UserInterfaceDelegate.hpp>`
**Namespace:** `bg2e::ui`

`UserInterface` owns the Dear ImGui context and its SDL2 + Vulkan backends,
and exposes the four lifecycle hooks that `app::MainLoop` calls per frame.
`UserInterfaceDelegate` is the application-side interface injected into it.

```cpp
class BG2E_API UserInterface {
public:
    void init(render::Engine *);
    void processEvent(SDL_Event * event);
    void newFrame();
    void draw(VkCommandBuffer cmd, VkImageView targetImageView);
    void cleanup();

    void setFrameOverride(std::function<void()> fn);
    void clearFrameOverride();

    void setDelegate(std::shared_ptr<UserInterfaceDelegate> delegate);

    static float getScale();
    static void setScale(float scale);
    // ...
};

class BG2E_API UserInterfaceDelegate {
    friend class app::MainLoop;
public:
    virtual void init(bg2e::render::Engine*, UserInterface*) {}
    virtual void drawUI();                       // default: DemoWindow::draw()

    uint32_t uiWidth() const;                    // viewport size, kept by MainLoop
    uint32_t uiHeight() const;
};
```

---

## `UserInterfaceDelegate`

The application-side interface injected via `UserInterface::setDelegate()`
(and owned by `app::Application` as a `shared_ptr`). Implement `drawUI()` to
render your UI every frame:

```cpp
void drawUI() override        // called between NewFrame() and Render()
{
    _workspace.draw();        // or draw Window/Toolbar/panels directly
}
```

- `init(engine, ui)` runs once after `UserInterface::init()`;
  `uiWidth()`/`uiHeight()` already report the current viewport (updated by
  `MainLoop` on resize, so `swapchainResized` can drive `Workspace::resize`).
- The **default** `drawUI()` shows `DemoWindow` — always override it in real
  applications.
- Capture `this` freely in UI callbacks: the delegate lives as long as the
  `Application`.

---

## Lifecycle

The instance is owned by `app::MainLoop`; applications interact only through
the delegate. Timeline (see `lib/src/bg2e/app/MainLoop.cpp`):

```
run(application)
  ui->setDelegate(application->uiDelegate())
  ui->init(&engine)            // reads "ui" preferences, builds ImGui,
                               // then delegate->init(engine, ui)
  per frame:
    SDL event -> input delegate -> ui->processEvent(&event)
    ui->newFrame()             // ImGui::NewFrame + drawUI (or frame override) + Render
    render loop acquire/present:
      scene pass ...
      ui->draw(cmd, swapchainImageView)   // via renderUICallback
  ui->cleanup()                // writes uiScale back to preferences
```

`draw()` opens its own dynamic-rendering pass on the provided image view
(single color attachment), so it must be called **after** the scene has been
rendered into that view and while the command buffer is recording — which is
exactly what `RenderLoop::renderUICallback` guarantees.

### What `init()` creates

- A resettable command pool + one command buffer (destroyed via the engine
  `CleanupManager`).
- A signaled `VkFence` for UI synchronization.
- A dedicated `VkDescriptorPool` (1000 sets, all types) used by ImGui and by
  `TextureWidgets` descriptor bindings.
- ImGui context + `ImGui_ImplSDL2_InitForVulkan` + `ImGui_ImplVulkan_Init`
  configured for **dynamic rendering** with the swapchain color format.
- The base style (captured once) and the engine font
  `assets/DidactGothic-Regular.ttf` at 16 px.

---

## Frame override

`setFrameOverride(fn)` temporarily replaces `delegate->drawUI()`. It is the
mechanism behind `app::MainLoop::asyncLoad()`: the scene is paused, the
`Loader` overlay is drawn instead of the UI, and `clearFrameOverride()`
restores normal operation when the worker thread completes. Set/clear it only
between frames (never inside `drawUI()`).

---

## UI scale

- `setScale()` sets a static factor and flags a style refresh; `newFrame()`
  applies it before the frame: base style `ScaleAllSizes(scale)` +
  `io.FontGlobalScale = scale`. The base style is never lost, so lowering the
  scale restores the original layout.
- `getScale()` is read by every layout-aware widget (`Workspace`, toolbar
  heights, etc.).
- Persistence is automatic (`"ui"` preferences context, key `uiScale`);
  `UISettingsWindow` provides the user-facing slider.
- Note for layout math: `Workspace` multiplies logical sizes by the scale
  itself; when placing your own windows using `uiWidth()/uiHeight()` keep the
  same convention in mind.

---

## Event routing

`MainLoop` processes each SDL event in this order:

1. Window/system handling and `app::InputDelegate` callbacks.
2. `UserInterface::processEvent(event)` → `ImGui_ImplSDL2_ProcessEvent`.

ImGui then routes clicks/keys to widgets *inside itself*, but the engine input
delegate has already seen the event. Scene input handlers should therefore
verify the UI is not capturing input before acting (see
`doc/input_delegate.md` and the quick start pitfalls).

---

## See also

- [index.md — Integration with the main loop](index.md#integration-with-the-main-loop)
- [quick_start — Recipe 1](quick_start.md#recipe-1-wire-a-ui-into-an-application)
- [Loader](Loader.md) — the frame-override use case.
- [Window](Window.md) — what delegates typically draw.
