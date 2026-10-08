# Step 03: Define the initial experimental draw contracts

## New files
Add public headers under `lib/include/bg2e/draw/`: EngineConfig.hpp, Engine.hpp, FrameContext.hpp, RenderLoopDelegate.hpp, RenderLoop.hpp and all.hpp. Add Engine.cpp and RenderLoop.cpp under `lib/src/bg2e/draw/`. Expose draw/all.hpp through the existing umbrella inclusion convention without including backend-specific headers.

## EngineConfig
Define backend (default Vulkan), debug (default false), applicationName (default empty; MainLoop appId is the fallback), colorFormat (B8G8R8A8_UNORM) and depthFormat (D32_SFLOAT). Use gpu enums. Do not add production Renderer configuration, Vulkan allocators, swapchain handles or a user-supplied execution wrapper.

Backend configuration selects low-level GPU API only. The run overload selects the high-level path.

## Engine
Use PImpl, an out-of-line destructor, deleted copy operations and no implicit copying of GPU ownership. Declare init(SDL_Window*, gpu::Backend&, const EngineConfig&), cleanup, backendType, instance, physicalDevice, device, surface and cleanupManager accessors. SDL_Window is non-owning. Backend and its shared Instance are non-owning; PhysicalDevice and Device are exclusively owned. Surface is exclusively owned for this single-window milestone; use shared ownership only if a concrete API requires it.

The private implementation may initially contain initialization state without allocating GPU objects. init throws std::logic_error("draw GPU initialization is not implemented; complete milestone 02"). Object accessors throw when uninitialized. cleanup is safe for an uninitialized shell and repeat calls. Define every non-inline function now; do not leave unresolved symbols.

## Delegate and frame context
Define a separate draw::RenderLoopDelegate, leaving the production delegate untouched. Provide a virtual destructor, init(Engine*), initScene(), resize(gpu::Size2D), update(const FrameContext&), pure virtual render(const FrameContext&) and cleanup(). The base stores a non-owning Engine pointer. No scene headers, production types or native backend types are included.

FrameContext contains an Engine reference, gpu::CommandBuffer reference, gpu::Image reference for the 3D color target, pixel extent, uint64_t frameNumber, uint32_t frameSlot and float deltaSeconds. These are borrowed for the current operation. The image describes a retained scene target, not an assumption that the scene renders directly into the acquired presentation image.

No initFrameResources/descriptor-pool callback is added. Resources and resource-set slot ownership remain with consuming objects. A future pre-existing scene delegate can add loadScene hooks independently; do not invent a new scene graph now.

## RenderLoop
Define setDelegate, init(Engine*), initScene, frame(deltaSeconds, ui::UserInterface&), requestResize, pauseScene, resumeScene and cleanup. Forward-declare UserInterface in the public header. RenderLoop owns coordination state and, in milestone 02, the retained scene target. Engine does not own the UI.

Define a scene-dirty flag and requestSceneFrame(). Pause suppresses scene delegate update/render and retains the last scene image; UI/presentation can continue. The first frame requires a valid initial scene image. For milestone 01, frame/init throw a descriptive not-implemented exception and cleanup remains safe.

## End state
Experimental contracts exist without pretending to render. Production does not depend on their implementation details. No new GPU behavior, tests or build invocation is included.
