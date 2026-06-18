# Plan Status

## Step 01 completed: RenderLoop scene pause / resume
Date: 2026-06-18
Changes:
- `lib/include/bg2e/render/RenderLoop.hpp`: Added `#include <glm/glm.hpp>`, public methods `pauseScene()` / `resumeScene() / isScenePaused()`, private members `_scenePaused` and `_sceneClearColor`, and protected helper `cmdClearColorImage()`
- `lib/src/bg2e/render/RenderLoop.cpp`: Implemented `pauseScene()`, `resumeScene()` with `device().waitIdle()`, helper `cmdClearColorImage()` using `vkCmdClearColorImage`, and wrapped MSAA + non-MSAA branches in `acquireAndPresent()` with `_scenePaused` conditional that clears images instead of rendering

## Step 02 completed: UserInterface frame-override mechanism
Date: 2026-06-18
Changes:
- `lib/include/bg2e/ui/UserInterface.hpp`: Added `#include <functional>`, public methods `setFrameOverride()` / `clearFrameOverride()`, protected member `_frameOverride`
- `lib/src/bg2e/ui/UserInterface.cpp`: Implemented `setFrameOverride()` / `clearFrameOverride()`, modified `newFrame()` to check `_frameOverride` first and call it instead of `_delegate->drawUI()` when set

## Step 03 completed: Thread-safe Loader widget
Date: 2026-06-18
Changes:
- `lib/include/bg2e/ui/Loader.hpp`: Created new header with class Loader featuring thread-safe setMessage/getMessage/setProgress/getProgress/draw methods using std::recursive_mutex
- `lib/src/bg2e/ui/Loader.cpp`: Created new implementation drawing a centered non-closeable ImGui window with message text and progress bar
- `lib/include/bg2e/ui/all.hpp`: Added `#include <bg2e/ui/Loader.hpp>`

## Step 04 completed: asyncLoad() + completion queue
Date: 2026-06-18
Changes:
- `lib/include/bg2e/app/MainLoop.hpp`: Added includes for Loader, thread, mutex, queue, glm; public method `asyncLoad()`, private members `_loader`, `_mainThreadQueueMutex`, `_mainThreadQueue`, and `drainMainThreadQueue()`
- `lib/src/bg2e/app/MainLoop.cpp`: Implemented `asyncLoad()` (pauses scene, sets frame override, spawns worker thread with completion callback), `drainMainThreadQueue()` (swaps and drains queue without holding lock during callbacks), added `drainMainThreadQueue()` call in `run()` after `executeSafeUpdateScene()`
