# Step 02: Route graphics operations through the extracted implementation

## MainLoop ownership
Replace direct Engine/RenderLoop storage with a unique_ptr to the forward-declared internal GraphicsExecution. Keep the destructor defined out of line with the complete interface available. Keep InputManager, UserInterface, Loader, preferences and scheduling state in MainLoop.

Before removing protected engine/loop members, search repository MainLoop subclasses and direct access to these members. Do not confuse RenderLoopDelegate::_engine use with MainLoop member use. If existing application code accesses a MainLoop protected member, preserve a production-only compatibility reference/access bridge with the same usable type and lifetime; document it rather than rewriting application code. Do not retain a second independently initialized engine.

## Common execution helper
Add a private runInternal helper accepting Application and an already-created GraphicsExecution. Existing run(Application*) constructs the production implementation and delegates to this helper. Reject a null Application before dereferencing it. Call execution.validate before SDL initialization.

Pass execution.windowType into createSDLWindow and map it to SDL_WINDOW_VULKAN or SDL_WINDOW_METAL while preserving every other window configuration flag and preference. Only Vulkan is selected by reachable execution in this step.

Preserve SDL hint/driver setup, viewport initialization, input delegate setup, scene initialization order, event handling, resize debounce, foreground/background frame limits, delta calculation, timeout execution and preference persistence.

## Required replacements
- Engine/RenderLoop/UI initialization becomes execution.initialize; input delegate setup remains in MainLoop; descriptor/scene startup becomes execution.initializeScene.
- Resize events call execution.requestResize at the same point as the old updateSwapchainSize call.
- Replace newFrame/resize notification/setDelta/newFrame/acquireAndPresent with execution.frame(delta.count(), !resizing).
- executeSafeUpdateScene keeps token handling and queue swapping unchanged, but calls execution.waitIdle before invoking work.
- asyncLoad retains Loader/frame override/thread/completion handling; pause and resume use execution.pauseScene/resumeScene.
- Shutdown preserves exit timers and preference storage, then execution.waitIdle, execution.cleanup, SDL_DestroyWindow.

Do not move generic scheduling into the graphics implementation or duplicate the event loop. Retain the existing minimum resize interval and the existing UI preparation during debounce. Do not opportunistically change SDL_Quit behavior, input capture policies or detached-worker ownership.

## End state
Production MainLoop has no direct Vulkan command callback or descriptor orchestration. These live in its production execution implementation. Existing run calls and application source remain unchanged. All common helpers use the selected execution consistently. No test or build invocation is part of this step.
