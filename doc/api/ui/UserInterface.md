# UserInterface and UserInterfaceDelegate

**Header:** `<bg2e/ui/UserInterface.hpp>`, `<bg2e/ui/UserInterfaceDelegate.hpp>`
**Namespace:** `bg2e::ui`

`UserInterface` manages the Dear ImGui context, SDL2 and a private UI renderer,
with render/Vulkan and draw/Vulkan or draw/Metal entry points,
and exposes the four lifecycle hooks that `app::MainLoop` calls per frame.
`UserInterfaceDelegate` is the application-side interface injected into it.

```cpp
class BG2E_API UserInterface {
public:
    UserInterface();
    ~UserInterface();
    void init(render::Engine *);
    void init(draw::Engine *);
    bool initialized() const;
    void processEvent(SDL_Event * event);
    void newFrame();
    void newFrame(gpu::CommandBuffer&, gpu::SurfaceFrame&);
    void draw(VkCommandBuffer cmd, VkImageView targetImageView);
    void draw(gpu::CommandBuffer&, gpu::SurfaceFrame&);
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
    virtual ~UserInterfaceDelegate() = default;
    virtual void init(bg2e::render::Engine*, UserInterface*) {}
    virtual void init(bg2e::draw::Engine*, UserInterface*) {}
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

This is the lifecycle of a rendered frame, not an unconditional iteration
rate. If `app::MainLoop` background limiting is active while the window is
unfocused, entire UI frames are skipped together with scene update, rendering,
and presentation. SDL events continue to be processed between deadlines, and
the next UI frame reflects the accumulated application state. Code that needs
a prompt background frame can call `app::MainLoop::requestFrame()`.

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
  `UISettingsWindow` provides the user-facing slider. Its optional application
  settings section can also expose `MainLoop` background scheduling; initialize
  it with `init(true)` to enable that section.
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

## Experimental draw integration

`init(draw::Engine*)` configures UI rendering for Vulkan or Metal; Metal requires
macOS. UserInterfaceDelegate supplies `init(draw::Engine*, UserInterface*)` and
`init(render::Engine*, UserInterface*)` overloads for the respective engine
contexts. Both use drawUI() to describe widgets.
UI backend storage lives in PImpl, with no Metal or ImGui declarations in public
UI headers and no dependency from gpu to ui.

After successful acquisition and slot synchronization, MainLoop's draw adapter
calls `newFrame(command, frame)` before scene commands. It prepares renderer
metadata, SDL and ImGui, executes drawUI or frameOverride, then ImGui::Render.
`draw(command, frame)` must compose the same prepared command/frame pair after
the scene copy. It opens a color-only LOAD/STORE overlay pass, materializes
Vulkan's lazy native rendering scope, emits draw data and closes that scope.
UI does not submit or present; RenderLoop controls the surrounding transitions.

Metal initializes SDL with InitForMetal and uses the pinned ImGui backend's
supported Metal-cpp bindings. Preparation uses a color-only descriptor matching
the acquired texture's format/sample count, without opening an encoder. The
overlay opens a separate compatible LOAD/STORE pass, borrows its descriptor,
materializes the command buffer's encoder and records ImGui draw data. GPU ends
the scope exactly once and owns encoder lifetime. Metadata is updated for each
acquired texture and recreated on surface-generation/configuration changes.

Vulkan uses the actual surface color format and swapchain image count rather
than frames in flight. Surface capabilities determine a minimum image count
satisfying ImGui's requirement of at least two. A format/count change drains
GPU work before reinitializing the renderer. Pointer-backed configuration stays
alive in the private backend state. Unavailable drawables do not prepare UI;
scene pause does not suppress UI preparation. Viewport dimensions remain logical
window units.

## Context ownership and shutdown

For production, `cleanup()` persists preferences and the engine's registered
callback performs renderer, SDL and context shutdown in the registered lifecycle order.
The callback retains lifecycle state without capturing the UserInterface
wrapper. For draw, stop producers and call `cleanup()` before destroying Engine;
cleanup waits for completion and shuts down only initialized backend stages.

Context destruction resets static style/font bookkeeping. Initialization
failures clean started stages and preserve the original exception; repeated
cleanup is safe. `processEvent` and frame operations guard an uninitialized
context. UserInterface owns its lifecycle and cannot be copied. Only one ImGui
context is active at a time. Scene/texture editors that consume render resources
require render-compatible objects; selecting draw does not adapt those widgets.

## See also

- [index.md — Integration with the main loop](index.md#integration-with-the-main-loop)
- [quick_start — Recipe 1](quick_start.md#recipe-1-wire-a-ui-into-an-application)
- [Loader](Loader.md) — the frame-override use case.
- [Window](Window.md) — what delegates typically draw.

## Architecture reference

See [MainLoop/draw integration](../../architecture/MainLoop_render_draw_gpu.md#ui-interoperability-lives-in-ui)
for private Vulkan/Metal source excerpts, acquired-frame preparation/composition,
encoder ownership, context lifetime and render shutdown ownership.
Scene-dependent UI components must use resources compatible with their engine
contract.
