# Step 01: Extract the production graphics execution implementation

## Inputs and files
Read `lib/include/bg2e/app/MainLoop.hpp`, `lib/src/bg2e/app/MainLoop.cpp`, the production Engine/RenderLoop interfaces and `ui/UserInterface.cpp`. Add `lib/src/bg2e/app/detail/GraphicsExecution.hpp` and `RenderGraphicsExecution.cpp`. Keep this interface private to library sources; forward-declare it in MainLoop.hpp when the member is introduced in step 02.

## Interface
Define a virtual destructor and these operations:
- `gpu::WindowType windowType() const`: Vulkan for production.
- `void validate(const Application&) const`: inspect configured delegates without allocating SDL/GPU resources.
- `void initialize(SDL_Window*, Application&, ui::UserInterface&)`.
- `void initializeScene()`.
- `void requestResize()`: record a request, do not recreate a surface during SDL event dispatch.
- `void frame(float deltaMilliseconds, bool renderingAllowed)`.
- `void waitIdle()`.
- `void pauseScene(const glm::vec4&)` and `void resumeScene()`.
- `void cleanup()`.

Declare factory functions returning `std::unique_ptr<GraphicsExecution>`; define the production factory in this step. Do not declare an experimental factory until its definition is added later.

The outer frame time remains milliseconds to preserve production semantics. The draw implementation will convert explicitly to seconds.

## Production mapping
The concrete implementation owns Engine and RenderLoop. initialize reproduces this order: Engine::init(window); UI delegate binding and UserInterface::init(engine); render delegate binding and RenderLoop::init(engine); Vulkan renderUICallback binding. The callback receives the existing native parameters and calls the same UserInterface::draw overload.

initializeScene initializes the main descriptor pool and then calls RenderLoop::initScene. requestResize calls Engine::updateSwapchainSize. frame calls Engine::newFrame and RenderLoop::swapchainResized only when renderingAllowed; it always assigns delta and calls UserInterface::newFrame, and calls acquireAndPresent only when renderingAllowed. This reproduces the existing behavior during resize debounce.

pause/resume call the existing RenderLoop methods. waitIdle calls the production device. cleanup retains the existing order RenderLoop::cleanup, UserInterface::cleanup, Engine::cleanup. MainLoop performs the preceding waitIdle operation.

Keep engine and UI references valid through engine cleanup: existing production cleanup closures refer to UI state. Clear callback captures only after their last possible use. Implement initialization stage tracking so cleanup does not access an engine that was never initialized; do not refactor render internals to provide this tracking.

## Boundaries
Do not route MainLoop yet: this step introduces a complete, initially unused implementation. No new public factory is exposed. Do not change render or UserInterface code. Do not remove production descriptor initialization because draw will use a different model.

## End state
The production execution implementation has complete definitions. Existing MainLoop execution remains unchanged until step 02. No test or build invocation is part of this step.
