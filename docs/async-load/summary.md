# Async Scene Loading — Implementation Plan

## Goal

Enable 3D model and scene loading to happen on a background thread while the
application keeps rendering (showing only a loading screen) and handling events
normally. After loading completes the scene resumes with no visible glitch.

## High-level design

```
RenderLoop   ── scene pause / resume + clear colour
UserInterface ── frame-override mechanism
ui::Loader   ── thread-safe modal progress window
MainLoop     ── asyncLoad() orchestrator + main-thread completion queue
```

### Invariants

* The GPU is always drained (`device().waitIdle()`) before pausing **and** before
  resuming, so scene resources are never accessed while they are being mutated
  from the loader thread.
* The loader thread never touches Vulkan or ImGui directly — it only updates
  `Loader` fields through thread-safe accessors.
* Frame-override completely replaces the application delegate draw; no changes
  are required in application code.
* `asyncLoad` is accessible as `app::MainLoop::current()->asyncLoad(lambda)`.

## Steps

| # | File(s) changed | Description |
|---|---|---|
| 01 | `render/RenderLoop.hpp/.cpp` | [Scene pause / resume + clear](step-01-renderloop-pause-resume.md) |
| 02 | `ui/UserInterface.hpp/.cpp` | [Frame-override mechanism](step-02-userinterface-frame-override.md) |
| 03 | `ui/Loader.hpp/.cpp`, `ui/all.hpp` | [Thread-safe Loader window](step-03-ui-loader.md) |
| 04 | `app/MainLoop.hpp/.cpp` | [asyncLoad() + completion queue](step-04-mainloop-asyncload.md) |

Each step leaves the codebase in a compilable state.

## Public API (final)

```cpp
// Pause / resume scene rendering (independent of async load)
renderLoop.pauseScene(glm::vec4 clearColor = {0,0,0,1});
renderLoop.resumeScene();
renderLoop.isScenePaused();   // bool

// Thread-safe loader widget (independent of async load)
ui::Loader loader;
loader.setMessage("Importing model...");   // thread-safe
loader.setProgress(0.42f);               // thread-safe
loader.draw();                           // call from ImGui frame

// Async load (combines all of the above)
app::MainLoop::current()->asyncLoad(
    [](bg2e::ui::Loader* loader) {
        loader->setMessage("Loading geometry...");
        loader->setProgress(0.0f);
        // ... heavy work ...
        loader->setProgress(1.0f);
    },
    glm::vec4{0.05f, 0.05f, 0.05f, 1.f}   // optional clear colour
);
```
