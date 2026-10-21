# MainLoop architecture: draw application execution

MainLoop connects application delegates, window/events, draw coordination and UI.
The windowed and headless application entry points now use draw exclusively.

## Production framework

Draw is the sole application rendering path. Selecting Vulkan selects the GPU
backend beneath draw. Retired rendering wrappers and old application APIs have
been removed.

## Responsibilities and API boundaries

MainLoop owns window/input/timers/background scheduling, UserInterface and joined
async work. DrawGraphicsExecution owns draw Engine/RenderLoop; Engine owns the
GPU context, and RenderLoop coordinates retained scene color and presentation.
Applications supply draw graphics, input and UI delegates. GPU primitives remain
independent of higher-level draw/scene/UI engine namespaces.

## Public selection and delegate contracts

```cpp
MainLoop loop("org.example.editor");
application.init(argc, argv);
return loop.run(&application); // Metal on macOS, Vulkan elsewhere.
```

The explicit EngineConfig overload overrides backend/formats/validation.
Application stores one draw::RenderLoopDelegate, accessed by renderDelegate()
and setRenderDelegate(). Input/UI slots remain separate; passing nullptr clears
the graphics slot. No opposite-framework slot or experimental opt-in remains.
Validation rejects null application/graphics/UI/input and Metal outside macOS
before SDL/window allocation. Backend preparation fills an empty applicationName
from appId and retains the backend through context lifetime.

## Graphics execution strategies and shared responsibilities

The private GraphicsExecution interface and DrawGraphicsExecution implementation
provide validation, preparation, initialization, scene startup, resize, frame,
waitIdle, scene invalidation/pause/resume and cleanup. They are implementation
services, not a public replacement renderer API. MainLoop supplies milliseconds;
the draw adapter converts to seconds. Renderer switching/live settings remain
inside the existing draw delegate, with generation guards and stopped updates.

MainLoop event polling and Loader remain active while scene refresh is paused.
Safe-update callbacks drain conservatively and invalidate retained scene only
when executed. Async workers join before normal/exceptional graphics teardown.
CPU/GPU profiling association and narrowly typed final-color drain elision are
preserved; promotion introduces no new timing waits or resource rebuild policy.

## Draw Engine: context and ownership

DrawGraphicsExecution copies EngineConfig. Empty applicationName becomes the
MainLoop appId. `Factory::acquireBackend` atomically replaces/retains the global
backend under a mutex and rejects an existing external lease. Engine additionally
retains factory-owned backends; caller-owned backends remain borrowed. This
supports a controlled single shared-instance lifetime, not simultaneous active
engines for independent backends.

Source: [lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp](../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp), lines 97–102. Exact excerpt:

```cpp
    void prepare(const std::string& applicationId) override
    {
        if (_backend) return;
        if (_config.applicationName.empty()) _config.applicationName = applicationId;
        _backend = gpu::Factory::acquireBackend(_config.backend);
    }
```

Source: [lib/src/bg2e/draw/Engine.cpp](../../lib/src/bg2e/draw/Engine.cpp), lines 36–68. Exact excerpt:

```cpp
struct Engine::Impl {
    SDL_Window* window = nullptr;               // Non-owning
    std::shared_ptr<gpu::Backend> backendLease; // Pins factory-owned backends.
    gpu::Backend* backend = nullptr;            // Non-owning
    gpu::Instance* instance = nullptr;          // Non-owning (shared with Backend)
    std::unique_ptr<gpu::PhysicalDevice> physicalDevice;
    std::unique_ptr<gpu::Device> device;
    std::unique_ptr<gpu::WindowSurface> surface;
    std::unique_ptr<gpu::CleanupManager> cleanupManager;
    EngineConfig config;
    bool initialized = false;
    bool instanceCreationStarted = false;
    bool deviceCreationStarted = false;
    bool cleaning = false;

    void reset()
    {
        cleanupManager.reset();
        surface.reset();
        device.reset();
        physicalDevice.reset();
        instance = nullptr;
        backend = nullptr;
        backendLease.reset();
        instanceCreationStarted = false;
        deviceCreationStarted = false;
        cleaning = false;
        window = nullptr;
        config = EngineConfig{};
        initialized = false;
    }
};
```

Engine initialization checks window/backend compatibility and shared Instance
availability, configures name/debug, creates the Instance and WindowSurface,
chooses a suitable PhysicalDevice, and creates Device. Device::create also creates
the surface render target. Engine then creates CleanupManager and marks itself
initialized. The Instance wrapper belongs to Backend; Engine owns its initialized
lifetime and cleans it. Window/backend references are borrowed, with the optional
lease keeping a factory backend alive. Stage flags support rollback while
preserving the original initialization exception.

Source: [lib/src/bg2e/draw/Engine.cpp](../../lib/src/bg2e/draw/Engine.cpp), lines 79–128. Exact excerpt:

```cpp
void Engine::init(SDL_Window* window, gpu::Backend& backend, const EngineConfig& config)
{
    if (_impl->initialized || _impl->instanceCreationStarted || _impl->cleaning)
        throw std::logic_error("draw::Engine is already initialized or changing lifecycle state.");
    if (!window) throw std::invalid_argument("draw::Engine::init requires an SDL window.");
    if (backend.backendType() != config.backend)
        throw std::invalid_argument("draw::Engine backend does not match EngineConfig::backend.");

    try
    {
        _impl->backendLease = gpu::Factory::retainBackend(backend);
        _impl->window = window;
        _impl->backend = &backend;
        _impl->config = config;
        _impl->instance = backend.sharedInstance();
        if (!_impl->instance) throw std::runtime_error("Draw backend returned no Instance.");
        // The backend owns the wrapper; this Engine owns its initialized lifetime.
        // Do not overwrite an instance used by another context.
        if (_impl->instance->presentationMode() != gpu::PresentationMode::Undefined)
            throw std::logic_error("Draw backend shared Instance is already in use.");
        _impl->instanceCreationStarted = true;
        _impl->instance->setApplicationName(config.applicationName);
        _impl->instance->enableDebugMode(config.debug);
        _impl->instance->create(window);

        _impl->surface = backend.createWindowSurface(_impl->instance, config.colorFormat, config.depthFormat);
        if (!_impl->surface)
            throw std::runtime_error("Draw backend returned no WindowSurface.");
        _impl->physicalDevice = backend.createPhysicalDevice();
        if (!_impl->physicalDevice) throw std::runtime_error("Draw backend returned no PhysicalDevice.");
        _impl->physicalDevice->choose(*_impl->instance, *_impl->surface);
        if (!_impl->physicalDevice->isValid()) throw std::runtime_error("No suitable draw GPU device.");
        _impl->device = backend.createDevice();
        if (!_impl->device) throw std::runtime_error("Draw backend returned no Device.");
        _impl->deviceCreationStarted = true;
        // Device::create also creates the surface render target.
        _impl->device->create(_impl->instance, _impl->physicalDevice.get(), _impl->surface.get());
        if (!_impl->device->isValid()) throw std::runtime_error("Draw GPU Device creation failed.");
        // Vulkan surface validity includes its swapchain, created by Device.
        if (!_impl->surface->isValid()) throw std::runtime_error("Draw surface render target creation failed.");
        _impl->cleanupManager = std::make_unique<gpu::CleanupManager>(_impl->surface.get());
        _impl->initialized = true;
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try { cleanup(); } catch (...) { /* Preserve the initialization failure. */ }
        std::rethrow_exception(error);
    }
}
```

## Draw adapter: delegate and UI assembly

The adapter initializes Engine, installs the draw delegate, initializes its
coordinator and UI, then supplies preparation/composition callbacks. Engine has
no UI ownership. Neither abstract gpu nor draw scene contracts require ImGui or
native Metal declarations.

Source: [lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp](../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp), lines 104–129. Exact excerpt:

```cpp
    void initialize(
        SDL_Window* window,
        Application& application,
        ui::UserInterface& userInterface
    ) override
    {
        if (!_backend) throw std::logic_error("Draw backend has not been prepared.");
        _engine.init(window, *_backend, _config);
        _engineInitialized = true;
        _userInterface = &userInterface;
        _renderLoop.setDelegate(application.renderDelegate());
        _renderLoop.init(&_engine);
        _lastDrawableSize = drawableSize();
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        application.uiDelegate()->setInitialSize(uint32_t(width), uint32_t(height));
        userInterface.setDelegate(application.uiDelegate());
        userInterface.init(&_engine);
        _uiInitialized = true;
        _renderLoop.setUIFramePreparationCallback([this](auto& command, auto& frame) {
            _userInterface->newFrame(command, frame);
        });
        _renderLoop.setUICompositionCallback([this](auto& command, auto& frame) {
            _userInterface->draw(command, frame);
        });
    }
```

Source: [lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp](../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp), lines 142–155. Exact excerpt:

```cpp
    void frame(float deltaMilliseconds, bool renderingAllowed) override
    {
        if (!renderingAllowed || !_engineInitialized) return;
        const auto size = drawableSize();
        if (size.isZero()) return;
        if (_resizePending || size != _lastDrawableSize)
        {
            _engine.surface()->resize(size);
            _renderLoop.requestResize();
            _lastDrawableSize = size;
            _resizePending = false;
        }
        _renderLoop.frame(deltaMilliseconds / 1000.0f);
    }
```

The adapter queries drawable pixel size using SDL_Metal_GetDrawableSize or
SDL_Vulkan_GetDrawableSize; logical SDL window dimensions are used for initial
UI viewport. Resize events mark pending work; positive drawable sizes trigger
surface recreation at the frame boundary. Zero sizes skip rendering. Surface
generation and the actually acquired image's dimensions/format drive downstream
scene and UI refresh, including backend-initiated swapchain recreation.
MainLoop keeps elapsed milliseconds; draw converts them to seconds.

## Retained scene, independent UI and frame slots

RenderLoop owns one persistent scene color image and per-in-flight-slot retained
command/frame wrappers. The scene image is shared across slots, not allocated
once per frame. Scene rendering and copies are on the same graphics queue;
Vulkan layout/access dependencies and Metal tracked hazards order GPU accesses.
This does not provide automatic cross-queue synchronization for objects using multiple queues.

Each positive available frame:

1. Acquire a presentation image and complete previous submissions occupying its
   resource slot; unavailable acquisition returns without advancing the slot.
2. Recreate retained color if size, format or generation changed (drain all
   users first), and notify the scene delegate of pixel extent.
3. Retain a fresh command/frame wrapper in the slot, begin recording and prepare UI.
4. If dirty and unpaused, call update, clear retained color, and invoke render.
   Initial/recreated color is cleared even while paused. The delegate closes its
   own rendering/compute scopes; the scene contract supplies no depth target.
5. Copy retained scene color to presentation color and compose UI as a separate
   color-only LOAD/STORE pass.
6. Record presentation, end recording, submit asynchronously, advance the
   surface slot/counter and poll deferred cleanup.

This excerpt is the concrete GPU submission tail, not pseudocode:

Source: [lib/src/bg2e/draw/RenderLoop.cpp](../../lib/src/bg2e/draw/RenderLoop.cpp), lines 152–165. Exact excerpt:

```cpp
    command.transition(_impl->sceneColor.get(), gpu::ImageLayout::TransferSrc);
    command.transition(presentationColor, gpu::ImageLayout::TransferDst);
    command.copyImage(_impl->sceneColor.get(), presentationColor);
    command.transition(presentationColor, gpu::ImageLayout::ColorAttachment);
    if (_uiComposition) _uiComposition(command, *frame);
    if (command.hasActiveScope()) throw std::logic_error("Draw UI composition left an active command scope.");
    command.transition(presentationColor, gpu::ImageLayout::Present);
    surface->present(&command);
    command.end();
    queue.submit(&command);
    _impl->colorInitialized = true;
    if (refresh && revision == _sceneRevision) _sceneDirty = false;
    surface->endFrame(frame.get());
    _engine->cleanupManager().flushDeferred();
```

The dirty revision is captured before scene callbacks. A request made during
scene/UI callbacks changes the revision, preventing the just-submitted frame
from clearing a newer request. The [FrameContext](../api/draw/FrameContext.md)
references are borrowed only for the callback; owning resources must not store
references to that transient context.

Consuming objects should hold persistent UBO/resource-set rings indexed by
`frameSlot`. Acquisition makes the reused slot safe before callbacks. Swapchain
image index, frame slot and monotonically increasing frame number are distinct.
There is no central create/destroy-per-frame descriptor phase. This does not
remove GPU synchronization requirements for shared or cross-queue resources.

## UI coordination and GPU native interoperability

UserInterface selects the SDL platform backend and private renderer from
Engine::backendType. Shared implementation state stores the context, selected
renderer, initialized/prepared flags and borrowed draw Engine. New-frame and draw
calls require the same acquired command/frame pair. Only one current ImGui
context is supported. UI wrappers keep ImGui types out of public headers.

The UI adapters delegate to gpu::GuiRenderer using borrowed GPU primitives.
The Vulkan bridge owns its descriptor pool, native instance/device/queue handles
and dynamic rendering configuration. Surface generation, count and format
changes refresh backend state; renderer recreation drains GPU users first.
`materializeRenderPass()` starts the GPU wrapper's lazy dynamic render pass
before the native ImGui draw call. No render callback API remains.


Source: [lib/src/bg2e/gpu/vk/GuiRenderer.cpp](../../lib/src/bg2e/gpu/vk/GuiRenderer.cpp). Exact excerpt:

```cpp
    void draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame) override
    {
        if (!_device || !frame.isValid() || !frame.colorImage())
        {
            throw std::logic_error("Draw UI requires a valid presentation target.");
        }
        auto& vkCommand = require<gpu::vk::CommandBuffer>(&command, "UI commands must use Vulkan.");
        if (command.hasActiveScope())
        {
            throw std::logic_error("UI overlay requires closed scene scopes.");
        }
        command.beginRendering(frame.colorImage()); // Color-only LOAD/STORE, no clear.
        vkCommand.materializeRenderPass();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCommand.handle());
        command.endRendering();
    }
```

The Metal GPU bridge uses guarded metal-cpp accessors in its source, derives a color-only
preparation descriptor from the acquired texture and caches generation/format/
sample count. Composition opens a distinct pass with no depth/stencil and LOAD/
STORE color actions. GPU owns the encoder and ends it once; the GPU GUI bridge borrows it:

Source: [lib/src/bg2e/gpu/metal/GuiRenderer.cpp](../../lib/src/bg2e/gpu/metal/GuiRenderer.cpp). Exact excerpt:

```cpp
    void draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame) override
    {
        auto* metalCommand = dynamic_cast<gpu::metal::CommandBuffer*>(&command);
        if (!_device || !_preparation || !metalCommand || !frame.isValid() || !frame.colorImage())
        {
            throw std::invalid_argument("Metal UI composition requires a prepared Metal frame.");
        }
        if (command.hasActiveScope())
        {
            throw std::logic_error("Metal UI composition requires closed scene scopes.");
        }
        {
            AutoreleaseScope autoreleaseScope;
            command.beginRendering(frame.colorImage()); // Color-only LOAD/STORE.
            auto* pass = metalCommand->renderPassDescriptor();
            auto* texture = pass->colorAttachments()->object(0)->texture();
            if (!texture || texture->pixelFormat() != _format || texture->sampleCount() != _sampleCount ||
                pass->depthAttachment()->texture() || pass->stencilAttachment()->texture())
            {
                throw std::logic_error("Metal UI preparation and overlay pass configurations differ.");
            }
            auto* encoder = metalCommand->materializeRenderEncoder();
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(),
                                           metalCommand->handle(),
                                           encoder);
            command.endRendering(); // GPU owns and ends the encoder exactly once.
        }
    }
```

UI does not commit or present command buffers. `renderPassDescriptor()` and
`materializeRenderEncoder()` are guarded backend-only gpu hooks, without a gpu
dependency on ui. Metal framework types do not appear in portable draw/app
contracts. The vendored Objective-C++ ImGui backend is selected only on Apple
and compiled with ARC; its provenance/local compatibility patches are recorded
in [METAL_BACKEND_ORIGIN](../../lib/third_party/imgui/METAL_BACKEND_ORIGIN.md).
UI components use canonical draw resources. ImGui/native implementation types
remain absent from public engine headers.

## Invalidation, pause and asynchronous work

`requestFrame()` is atomic/thread-safe and requests a presentation wakeup; it
does not invalidate draw's scene. `requestSceneFrame`, pause and resume are
main-thread operations during active run, forwarded through private execution.
A paused scene preserves last valid color while presentation/UI continue.
Requests stay pending until resume. Resize cannot preserve an obsolete-size
image, so a new target receives pause clear color.

Source: [lib/src/bg2e/app/MainLoop.cpp](../../lib/src/bg2e/app/MainLoop.cpp), lines 403–408. Exact excerpt:

```cpp
void MainLoop::requestSceneFrame()
{
    if (!_execution) throw std::logic_error("MainLoop::requestSceneFrame requires an active run.");
    _execution->requestSceneFrame();
    requestFrame();
}
```

Source: [lib/src/bg2e/app/MainLoop.cpp](../../lib/src/bg2e/app/MainLoop.cpp), lines 430–462. Exact excerpt:

```cpp
void MainLoop::executeSafeUpdateScene()
{
    std::vector<SafeUpdateSceneEntry> local;
    {
        std::lock_guard lock(_safeUpdateSceneMutex);
        if (_safeUpdateScene.empty()) return;
        std::swap(local, _safeUpdateScene);
    }
    // The execution exists only during an active run; outside a run there is
    // no GPU work in flight to wait for.
    if (_execution)
    {
        _execution->waitIdle();
    }
    bool updated = false;
    for (auto& entry : local)
    {
        if (!entry.hasToken)
        {
            entry.function();
            updated = true;
            continue;
        }

        auto token = entry.token.lock();
        if (token && token->alive->load())
        {
            entry.function();
            updated = true;
        }
    }
    if (updated && _execution) requestSceneFrame();
}
```

Safe-update enqueue is mutex-protected and requests a frame. At the main-thread
safe point it swaps the queue, waits through selected execution, skips expired
weak tokens, and invalidates draw only if callbacks ran. Token lifetime controls
cancellation; callbacks must respect object ownership and external GPU producers.
The wait is an intentional safe mutation boundary, not a per-frame wait.

Source: [MainLoop.cpp](../../lib/src/bg2e/app/MainLoop.cpp).

`asyncLoad` requires an active execution, pauses scene drawing and installs a Loader override when UI is ready. A loop-owned joinable worker executes loadFn and captures any exception. Its token-guarded safe-update completion clears the override, resumes scene drawing and calls onComplete on the main thread. Normal and exceptional run exit join workers before graphics/UI/Engine cleanup; exiting-run completion tokens expire.

Workers must not mutate live graph/GPU state or wait for main-thread callbacks in order to finish. Loader progress is thread-safe. Async loading has no nested pause ownership: overlapping loads/manual pauses require caller coordination because completion resumes the scene. Composer adds its own guarded IO and document-generation checks; shared worker ownership does not supply cancellation inside a third-party parser or asynchronous GPU realization.

## Synchronization and shutdown

Ordinary draw submission is asynchronous; occupied slot reuse waits, not every
just-submitted frame. Device::waitIdle is a lifecycle/safe mutation barrier with
a shared submission admission gate. Metal drains retained command completions;
Vulkan calls vkDeviceWaitIdle inside the same gate and latches records. See
[the full GPU synchronization implementation](../api/gpu/Submission_tracking_and_waitIdle.md).
The gate reopens when waitIdle returns: callers stop producers through the entire
resource mutation/destruction interval. A native handle submission outside the
gpu Queue API is not tracked by the Metal registry.

Draw cleanup attempts all established stages and reports the first error:
coordinator/delegate and retained targets, UI while Device still lives, Engine
resources/surface/device/Instance, then backend lease. Engine's concrete cleanup
also handles partial initialization:

Source: [lib/src/bg2e/draw/Engine.cpp](../../lib/src/bg2e/draw/Engine.cpp), lines 130–158. Exact excerpt:

```cpp
void Engine::cleanup()
{
    if (_impl->cleaning) return;
    _impl->cleaning = true;
    std::exception_ptr error;
    const auto attempt = [&error](auto&& operation) {
        try { operation(); } catch (...) { if (!error) error = std::current_exception(); }
    };
    // Callers stop frame/background producers before entering this method and
    // keep them stopped: waitIdle's submission gate reopens on return.
    if (_impl->deviceCreationStarted && _impl->device->isValid())
        attempt([&] { _impl->device->waitIdle(); });
    if (_impl->cleanupManager)
    {
        attempt([&] { _impl->cleanupManager->flushAllDeferred(); });
        attempt([&] { _impl->cleanupManager->flush(); });
        _impl->cleanupManager.reset();
    }
    // Keep the device alive while surfaces release frame command wrappers,
    // depth targets, swapchain images and presentation synchronization.
    if (_impl->surface) attempt([&] { _impl->surface->cleanup(); });
    _impl->surface.reset();
    if (_impl->deviceCreationStarted) attempt([&] { _impl->device->cleanup(); });
    _impl->device.reset();
    _impl->physicalDevice.reset();
    if (_impl->instanceCreationStarted) attempt([&] { _impl->instance->cleanup(); });
    _impl->reset();
    if (error) std::rethrow_exception(error);
}
```

## Source map and API scope

| Responsibility | Primary source |
|---|---|
| Shared lifecycle and scheduling | `lib/src/bg2e/app/MainLoop.cpp` |
| Strategy interface / adapters | `lib/src/bg2e/app/detail/GraphicsExecution.hpp`, `DrawGraphicsExecution.cpp` |
| Context and leases | `lib/src/bg2e/draw/Engine.cpp`, `lib/src/bg2e/gpu/Factory.cpp` |
| Retained scene and composition | `lib/src/bg2e/draw/RenderLoop.cpp` |
| UI context and coordination | `lib/src/bg2e/ui/UserInterface.cpp`, `ui/detail/GpuImGuiBackend.hpp` |
| Native GUI rendering | `lib/src/bg2e/gpu/vk/GuiRenderer.cpp`, `gpu/metal/GuiRenderer.cpp` |
| Completion registry and frame reuse | `lib/include/bg2e/gpu/detail/SubmissionState.hpp`, `gpu/SurfaceFrame.hpp`, backend Queue/Surface sources |

Draw scene callbacks expose retained color. RendererDeferred now owns the
internal depth/G-buffer graph and DefaultRenderLoopDelegate owns the runtime
renderer. DrawableComponent provides its canonical draw resource access;
GraphicsSettingsWindow uses the live draw facade. See the current
[renderer reference](../api/draw/RendererDeferred.md). Earlier abbreviated base
delegate excerpts omit its prepareFrame/surfaceChanged hooks.
[OffscreenApplication](../api/app/OffscreenApplication.md) owns a typed draw serial windowless job.
The [headless architecture](draw_offscreen_rendering.md) covers exact camera
samples, final-only conversion and completed readback outside MainLoop. There is no automatic multi-context backend
management or UI thread. Multiple layers currently call waitIdle during cleanup
and resize (MainLoop, coordinator/UI, Engine/surface); these overlapping drains
are conservative lifecycle boundaries. They are not present after every ordinary submission.

See [app API](../api/app/index.md), [draw API](../api/draw/index.md),
[gpu API](../api/gpu/index.md) and the [example](../../examples/draw/README.md).

## Model editor integration through M06

The [draw model editor](../apps/draw_model_edit.md) owns document generations,
dirty state, stopped edit queues and integrated bake scheduling above Engine.
Workspace composes its docked panels; utility and settings windows float.
Scene lighting editors use component reflection on temporary clones, then queue
mutation of the checked live target. Graphics preferences exclude scene and debug
properties. Graphics Settings contains no profiling controls.

Completed baking writes a temporary texture and automatically updates live and
CPU material definitions. A separately retained completed image keeps the preview
alive after baker invalidation. Complete resource export publishes native texture
sidecars independently of temporary files. The document graph is the bake root;
editor floor, stage light and camera are fixtures outside that graph. Composer and
standalone batch orchestration are not supplied by this application integration.


## Standalone draw lightmap application

The delivered [standalone bake CLI](../apps/draw_lightmap_generator.md) uses the
typed draw OffscreenApplication path, no camera frames and finish-owned serial
bake work. It reuses existing public effects rather than another graphics strategy.
Assembly and all job owners release before Engine cleanup; output uses its own
protected final-manifest contract. See [standalone ownership](draw_standalone_lightmaps.md)
for source/generation/complete model publication. Production/default branches
remain present; this addition does not remove render or promote draw globally.


The standalone device chooser keeps its preferences/widgets/selection in app.
`gpu::StandaloneGuiWindow` owns its native OpenGL window/context, SDL platform
backend and presentation. The chooser destroys it before its temporary ImGui
context and restores any previous context. `gpu::drawableSize` owns Vulkan/Metal
pixel-size queries used by DrawGraphicsExecution. See
[GPU native GUI/window bridges](../api/gpu/Gui_bridges.md) for ownership and limits.
