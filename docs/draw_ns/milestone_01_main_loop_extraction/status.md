# Plan Status

## Step 1 completed: Extract the production graphics execution implementation
Date: 2026-10-08
Changes:
- lib/src/bg2e/app/detail/GraphicsExecution.hpp: new internal `app::detail::GraphicsExecution` interface (windowType, validate, initialize, initializeScene, requestResize, frame, waitIdle, pauseScene/resumeScene, cleanup) plus `createRenderGraphicsExecution()` factory declaration.
- lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp: production implementation owning `render::Engine` and `render::RenderLoop`, reproducing MainLoop's initialization order, descriptor pool + initScene, resize request recording, frame semantics during resize debounce, pause/resume, waitIdle and ordered cleanup with initialization stage tracking.

## Step 2 completed: Route graphics operations through the extracted implementation
Date: 2026-10-08
Changes:
- lib/include/bg2e/app/MainLoop.hpp: removed `render::Engine`/`render::RenderLoop` members and their includes; added forward-declared `detail::GraphicsExecution`, `std::unique_ptr<detail::GraphicsExecution> _execution` member and private `runInternal` helper declaration.
- lib/src/bg2e/app/MainLoop.cpp: `run(Application*)` validates non-null application, creates the production execution and delegates to `runInternal`; `createSDLWindow` maps `gpu::WindowType` to SDL_WINDOW_VULKAN/METAL; engine/UI/render-loop init replaced by `execution->initialize` + `execution->initializeScene`; resize events call `execution->requestResize`; frame path replaced by `execution->frame(delta, !resizing)`; `executeSafeUpdateScene` and shutdown use `execution->waitIdle`; `asyncLoad` uses `execution->pauseScene`/`resumeScene`; shutdown order is waitIdle, cleanup, SDL_DestroyWindow.

## Step 3 completed: Define the initial experimental draw contracts
Date: 2026-10-08
Changes:
- lib/include/bg2e/draw/EngineConfig.hpp: new config struct (backend, debug, applicationName with appId fallback, colorFormat, depthFormat) using gpu enums.
- lib/include/bg2e/draw/Engine.hpp + lib/src/bg2e/draw/Engine.cpp: PImpl engine with out-of-line destructor, deleted copies, init/cleanup and backendType/instance/physicalDevice/device/surface/cleanupManager accessors; init throws `std::logic_error` (milestone 02 boundary), accessors throw when uninitialized, cleanup is idempotent.
- lib/include/bg2e/draw/FrameContext.hpp: borrowed per-frame context (engine, command buffer, retained color target, extent, frameNumber, frameSlot, deltaSeconds).
- lib/include/bg2e/draw/RenderLoopDelegate.hpp: new delegate with virtual destructor, init/initScene/resize/update/cleanup and pure virtual `render(const FrameContext&)`; stores non-owning Engine pointer.
- lib/include/bg2e/draw/RenderLoop.hpp + lib/src/bg2e/draw/RenderLoop.cpp: coordination shell with setDelegate, init, initScene, frame(deltaSeconds, ui::UserInterface&), requestResize, pauseScene/resumeScene, scene-dirty flag + requestSceneFrame, safe cleanup; init/initScene/frame throw descriptive not-implemented exceptions.
- lib/include/bg2e/draw/all.hpp: umbrella header for the draw namespace.
- lib/include/bg2e/all.hpp: added `#include <bg2e/draw/all.hpp>`.

## Step 4 completed: Add Application registration and the experimental run overload
Date: 2026-10-08
Changes:
- lib/include/bg2e/app/Application.hpp: added `_drawDelegate` slot, `drawDelegate()` getter, `setRenderDelegate(shared_ptr<draw::RenderLoopDelegate>)` overload and `setRenderDelegate(std::nullptr_t)`; non-null registration clears the other graphics slot, nullptr clears both.
- lib/src/bg2e/app/detail/GraphicsExecution.hpp: added `ensureRuntimeAvailable()` operation and `createDrawGraphicsExecution(const draw::EngineConfig&)` declaration.
- lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp: validate now uses `std::invalid_argument` with path-identifying messages and rejects a populated draw delegate slot; added no-op `ensureRuntimeAvailable`.
- lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp: new experimental execution owning `draw::Engine`/`draw::RenderLoop` with an EngineConfig copy; windowType maps the config backend; validate rejects render delegates, missing draw/input/UI delegates and Metal outside macOS (before SDL/GPU creation); `ensureRuntimeAvailable` throws `std::logic_error("Experimental draw execution requires milestone 02")`; frame converts milliseconds to seconds.
- lib/include/bg2e/app/MainLoop.hpp + lib/src/bg2e/app/MainLoop.cpp: added `run(Application*, const draw::EngineConfig&)` overload (no default argument) and `ensureRuntimeAvailable()` invocation after validate, before SDL initialization.

## Step 5 completed: Finalize lifecycle and source compatibility
Date: 2026-10-08
Changes:
- lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp: destructor now invokes the stage-guarded cleanup inside try/catch (noexcept cleanup during exception unwinding).
- lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp: same noexcept destructor cleanup guard.
- lib/src/bg2e/app/MainLoop.cpp: `executeSafeUpdateScene` guards a null `_execution` (no GPU work in flight outside a run); `asyncLoad` throws `std::logic_error` when called without an active run.
- docs/draw_ns/milestone_01_main_loop_extraction/summary.md: added completion notes recording final internal names, the absence of a protected-member bridge (replaced by `UserInterfaceDelegate::setInitialSize`), cleanup guarantees, the detached asyncLoad worker limitation and deferred milestone 02 topics.
