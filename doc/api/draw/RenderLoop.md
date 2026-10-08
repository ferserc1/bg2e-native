# RenderLoop

**Header:** `<bg2e/draw/RenderLoop.hpp>`  
**Namespace:** `bg2e::draw`

Coordinates experimental scene work, UI composition and presentation separately
from Engine's context ownership. Non-copyable. UI is borrowed, not engine-owned.
A retained scene target is planned; this milestone stores coordination state only.

```cpp
RenderLoop();
~RenderLoop();
void setDelegate(std::shared_ptr<RenderLoopDelegate> delegate);
std::shared_ptr<RenderLoopDelegate> delegate() const;
void init(Engine* engine);
void initScene();
void frame(float deltaSeconds, ui::UserInterface& userInterface);
void requestResize();
void pauseScene(const glm::vec4& clearColor = {0.f, 0.f, 0.f, 1.f});
void resumeScene();
bool isScenePaused() const;
void requestSceneFrame();
bool sceneDirty() const;
void cleanup();
```

## Implemented coordination state

Initially the scene is unpaused and dirty, with no pending resize and black
background `(0, 0, 0, 1)`.

| Operation | Current effect |
|-----------|----------------|
| `setDelegate()` | Stores a shared delegate; delegate() returns a shared pointer copy. |
| `requestResize()` | Sets the pending resize flag and marks the scene dirty. |
| `pauseScene(clearColor)` | Sets paused state and stores color; does not clear an image or change dirty state. |
| `resumeScene()` | Clears paused state and marks scene dirty. |
| `requestSceneFrame()` | Marks scene dirty; does not itself wake MainLoop or submit commands. |
| `cleanup()` | Calls the delegate cleanup if registered; then resets engine pointer and coordination flags/color. |

cleanup retains the registered delegate, so repeated calls invoke delegate
cleanup again. It does not establish GPU completion. The default destructor
does not invoke this cleanup method. If the delegate cleanup throws, subsequent
state resets in that call are not executed.

## Pending execution

- init stores the borrowed Engine pointer, then throws
  `std::logic_error("draw::RenderLoop initialization is not implemented; complete milestone 02")`.
- initScene throws
  `std::logic_error("draw::RenderLoop scene initialization is not implemented; complete milestone 02")`.
- frame throws
  `std::logic_error("draw::RenderLoop frame execution is not implemented; complete milestone 02")`.

No update/render callbacks, image acquisition, scene caching, UI composition or
presentation occur. The contract uses seconds for frame timing; the internal
MainLoop draw adapter converts its milliseconds input to seconds.

Future execution will permit scene pause with live UI over the last scene
image. That behavior is not implemented by storing the pause flag alone.
See [FrameContext](FrameContext.md) and [RenderLoopDelegate](RenderLoopDelegate.md).
