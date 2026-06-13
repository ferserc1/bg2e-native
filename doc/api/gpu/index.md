# GPU Abstraction Layer

The `bg2e::gpu` namespace provides a backend-agnostic abstraction layer for GPU
operations. It allows the engine to target multiple graphics APIs (Vulkan and
Metal) through a unified interface, using the Abstract Factory pattern to
decouple application code from any specific API.

## Architecture

The layer is organized into three tiers:

1. **Abstract interfaces** (`bg2e::gpu::*`) -- pure virtual classes that define
   the contract. Application code programs against these interfaces.
2. **Concrete backends** (`bg2e::gpu::vk::*` and `bg2e::gpu::metal::*`) --
   implement the interfaces using backend-specific types and expose additional
   accessors for raw handles when needed.
3. **Factory** (`bg2e::gpu::Factory`) -- entry point that creates the active
   backend singleton based on a `BackendType` enum.

### Class hierarchy

```
gpu::Factory                  (static, no inheritance)
gpu::Backend                  (abstract)
  +-- gpu::vk::Backend
  +-- gpu::metal::Backend
gpu::Instance                 (abstract)
  +-- gpu::vk::Instance
  +-- gpu::metal::Instance
gpu::Surface                  (abstract)
  +-- gpu::WindowSurface      (abstract)
  |     +-- gpu::vk::WindowSurface
  |     +-- gpu::metal::WindowSurface
  +-- gpu::OffscreenSurface   (concrete base)
        +-- gpu::vk::OffscreenSurface
        +-- gpu::metal::OffscreenSurface
gpu::PhysicalDevice           (abstract)
  +-- gpu::vk::PhysicalDevice
  +-- gpu::metal::PhysicalDevice
gpu::Device                   (abstract)
  +-- gpu::vk::Device
  +-- gpu::metal::Device
gpu::Queue                    (abstract)
  +-- gpu::vk::Queue
  +-- gpu::metal::Queue
gpu::ShaderModule             (abstract)
  +-- gpu::vk::ShaderModule
  +-- gpu::metal::ShaderModule
gpu::PipelineLayout           (abstract)
  +-- gpu::vk::PipelineLayout
  +-- gpu::metal::PipelineLayout
gpu::GraphicsPipeline         (abstract)
  +-- gpu::vk::GraphicsPipeline
  +-- gpu::metal::GraphicsPipeline
gpu::ComputePipeline          (abstract)
  +-- gpu::vk::ComputePipeline
  +-- gpu::metal::ComputePipeline
gpu::CommandBuffer            (abstract)
  +-- gpu::vk::CommandBuffer
  +-- gpu::metal::CommandBuffer
gpu::SurfaceFrame             (abstract)
  +-- gpu::vk::SurfaceFrame
  +-- gpu::metal::SurfaceFrame
gpu::Image                    (abstract)
  +-- gpu::vk::Image
  +-- gpu::metal::Image
gpu::Buffer                   (abstract)
  +-- gpu::vk::Buffer
  +-- gpu::metal::Buffer
gpu::ResourceSet              (abstract)
  +-- gpu::vk::ResourceSet
  +-- gpu::metal::ResourceSet
gpu::MeshGeneric<MeshT>       (template, not polymorphic)
  -- gpu::MeshP  / MeshPN  / MeshPC  / MeshPU
  -- gpu::MeshPNU / MeshPNC / MeshPNUC / MeshPNUT / MeshPNUUT
  -- gpu::Mesh   (alias for MeshPNUUT)
```

### Object creation flow

All objects are created through the `Backend` or `Device`:

```
Factory::init(BackendType)
    -> Backend* backend = Factory::backend()
        -> backend->sharedInstance()                   // singleton Instance
        -> backend->createWindowSurface(instance)      // or createOffscreenSurface()
        -> backend->createPhysicalDevice()
        -> backend->createDevice()
            -> device->createShaderModule(description)
            -> device->createPipelineLayout(description)
            -> device->createGraphicsPipeline(description)
            -> device->createComputePipeline(description)
            -> device->createImage(description)
            -> device->createSampler(description)
            -> device->createResourceSet(layout, setIndex)
            -> device->createBuffer(debugName)
            -> device->immediateSubmit(lambda)
```

Command buffers are created from queues:

```
device->graphicsQueue().createCommandBuffer()  // shared_ptr<CommandBuffer>
```

## Backends

| Backend | `BackendType`         | `WindowType`          | Platform     |
|---------|-----------------------|-----------------------|--------------|
| Vulkan  | `BackendType::Vulkan` | `WindowType::Vulkan`  | All          |
| Metal   | `BackendType::Metal`  | `WindowType::Metal`   | macOS only   |

On macOS you can choose either backend. On Linux and Windows, only Vulkan is
available. The `WindowType` enum is returned by `Backend::windowType()` and
determines which SDL window flags to use when creating a window.

## Initialization workflow

### Windowed rendering

The typical sequence for rendering into an OS window:

```cpp
#include <bg2e/gpu/all.hpp>

// 1. Select backend
gpu::Factory::init(gpu::BackendType::Vulkan);
auto* backend = gpu::Factory::backend();

// 2. Create SDL window using the backend's window type
SDL_Init(SDL_INIT_VIDEO);
Uint32 flags = (backend->windowType() == gpu::WindowType::Vulkan)
    ? SDL_WINDOW_VULKAN : SDL_WINDOW_METAL;
SDL_Window* window = SDL_CreateWindow("My App",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    800, 600, flags);

// 3. Create GPU instance and attach to window
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create(window);

// 4. Create window surface (initialized automatically)
auto surface = backend->createWindowSurface(instance);

// 5. Choose physical device
auto physicalDevice = backend->createPhysicalDevice();
physicalDevice->choose(*instance, *surface);

// 6. Create logical device
auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());
```

### Offscreen rendering

For headless rendering (no OS window), the workflow is similar but skips SDL
entirely and uses `OffscreenSurface` instead of `WindowSurface`:

```cpp
#include <bg2e/gpu/all.hpp>

// 1. Select backend
gpu::Factory::init(gpu::BackendType::Vulkan);
auto* backend = gpu::Factory::backend();

// 2. Create GPU instance (no window)
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create();  // headless overload

// 3. Create offscreen surface with fixed dimensions
auto surface = backend->createOffscreenSurface(instance, { 800, 600 });

// 4. Choose physical device
auto physicalDevice = backend->createPhysicalDevice();
physicalDevice->choose(*instance, *surface);

// 5. Create logical device
auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());
```

## Complete example: windowed device

This example creates a window, initializes the GPU, queries device properties,
and runs an event loop. Source: `examples/gpu/02_device/src/main.cpp`.

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <iostream>

static const char* deviceTypeString(
    bg2e::gpu::PhysicalDeviceProperties::DeviceType type)
{
    switch (type)
    {
        case bg2e::gpu::PhysicalDeviceProperties::DiscreteGPU:
            return "Discrete GPU";
        case bg2e::gpu::PhysicalDeviceProperties::IntegratedGPU:
            return "Integrated GPU";
        case bg2e::gpu::PhysicalDeviceProperties::VirtualGPU:
            return "Virtual GPU";
        case bg2e::gpu::PhysicalDeviceProperties::CPU:
            return "CPU";
        default:
            return "Unknown";
    }
}

int main(int argc, char** argv)
{
    using namespace bg2e;

    // On macOS, choose Metal or Vulkan; other platforms use Vulkan
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2)
            ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // Initialize the backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // Create an SDL window with the appropriate flags
    SDL_Init(SDL_INIT_VIDEO);
    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }
    SDL_Window* window = SDL_CreateWindow(
        "GPU Device Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, windowFlags);

    // Create GPU instance and attach to window
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    // Create surface
    auto surface = backend->createWindowSurface(instance);

    // Select the best physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    auto props = physicalDevice->properties();
    std::cout << "Selected GPU: " << props->name << std::endl;
    std::cout << "  Type:        " << deviceTypeString(props->deviceType) << std::endl;
    std::cout << "  Memory:      " << props->totalHeapMemoryMB << " MB" << std::endl;
    std::cout << "  Ray Tracing: "
              << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;

    // Create logical device and query queues
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    std::cout << "  Graphics queue: "
              << device->graphicsQueue().familyIndex() << std::endl;
    std::cout << "  Present queue:  "
              << device->presentQueue().familyIndex() << std::endl;
    std::cout << "  Transfer queue: "
              << device->transferQueue().familyIndex() << std::endl;

    // Event loop
    bool running = true;
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
        }
    }

    // Cleanup in reverse order
    device->cleanup();
    surface->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```

## Complete example: offscreen device

This example initializes the GPU without a window and creates an offscreen
surface for headless rendering. Source: `examples/gpu/03_offscreen_device/src/main.cpp`.

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
#include <iostream>

static const char* deviceTypeString(
    bg2e::gpu::PhysicalDeviceProperties::DeviceType type)
{
    switch (type)
    {
        case bg2e::gpu::PhysicalDeviceProperties::DiscreteGPU:
            return "Discrete GPU";
        case bg2e::gpu::PhysicalDeviceProperties::IntegratedGPU:
            return "Integrated GPU";
        case bg2e::gpu::PhysicalDeviceProperties::VirtualGPU:
            return "Virtual GPU";
        case bg2e::gpu::PhysicalDeviceProperties::CPU:
            return "CPU";
        default:
            return "Unknown";
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    using namespace bg2e;

    // On macOS, choose Metal or Vulkan; other platforms use Vulkan
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2)
            ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // Initialize the backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // Create GPU instance (no window)
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create();

    // Create offscreen surface
    auto surface = backend->createOffscreenSurface(instance, { 800, 600 });

    // Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    auto props = physicalDevice->properties();
    std::cout << "Selected GPU: " << props->name << std::endl;
    std::cout << "  Type:        "
              << deviceTypeString(props->deviceType) << std::endl;
    std::cout << "  Memory:      "
              << props->totalHeapMemoryMB << " MB" << std::endl;
    std::cout << "  Ray Tracing: "
              << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;

    // Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    std::cout << "  Graphics queue: "
              << device->graphicsQueue().familyIndex() << std::endl;
    std::cout << "  Present queue:  "
              << device->presentQueue().familyIndex() << std::endl;
    std::cout << "  Transfer queue: "
              << device->transferQueue().familyIndex() << std::endl;

    // Cleanup in reverse order
    device->cleanup();
    instance->cleanup();

    return 0;
}
```

## Render loop example

This example shows a complete render loop with vertex/index buffers, a compute
background, textures, resource sets, push constants, and frame presentation.
Source: `examples/gpu/05_simple_triangle/src/main.cpp`.

For a uniform-buffer example (rotating cube, per-frame `FrameResourceRing<gpu::Buffer>`,
no compute shader) see `examples/gpu/07_uniform_buffers/src/main.cpp`.

```cpp
// Build a pentagon mesh (position + texCoord0)
bg2e::geo::MeshPU meshData;
meshData.vertices = { /* 5 perimeter + 1 centre vertex */ };
meshData.indices  = { /* 15 indices, triangle fan */ };
meshData.submeshes = { { 0, 15 } };

gpu::MeshPU mesh;
mesh.setMeshData(meshData);
mesh.build(device.get());   // uploads vertex + index buffers to the GPU

// Add vertex layout to the pipeline description
gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader   = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout         = graphicsLayout.get();
pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
pipelineDesc.colorFormat    = colorFormat;
pipelineDesc.depthFormat    = depthFormat;
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(pipelineDesc);

// --- per-frame ---
auto frame = surface->beginFrame();
auto cmd   = graphicsQueue.createCommandBuffer("Frame");
cmd->begin();

// Compute pass: write gradient to the colour image
cmd->transition(frame->colorImage(), gpu::ImageLayout::General);
cmd->beginCompute();
cmd->bindPipeline(computePipeline.get());
cmd->bindResourceSet(computePipeline.get(), 0, computeSet.get());
cmd->dispatch(groupsX, groupsY, 1);
cmd->endCompute();

// Graphics pass
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
cmd->clearDepth(1.0f);
cmd->bindPipeline(pipeline.get());
cmd->bindResourceSet(pipeline.get(), 0, textureSet.get());
cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
mesh.draw(cmd.get());   // binds buffers and calls drawIndexed per submesh
cmd->endRendering();
cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
surface->endFrame(frame.get());
```

### Render loop structure

The render loop follows this pattern each frame:

```
1. Poll SDL events
2. surface->beginFrame()                -- acquire next swapchain image
3. cmd = graphicsQueue.createCommandBuffer()
4. cmd->begin()
5. cmd->transition(color, General)      -- for compute write
   cmd->beginCompute()
   cmd->bindPipeline(compute)
   cmd->bindResourceSet(...)
   cmd->dispatch(...)
   cmd->endCompute()
6. cmd->transition(color, ColorAttachment)
   cmd->transition(depth, DepthAttachment)
7. cmd->beginRendering(frame)
   cmd->clearDepth(1.0f)
   cmd->bindPipeline(graphics)
   cmd->bindResourceSet(...)
   cmd->pushConstants(...)
   mesh.draw(cmd)                        -- bindVertexBuffer + bindIndexBuffer + drawIndexed
   cmd->endRendering()
8. cmd->transition(color, Present)
9. surface->present(cmd)                -- record present
10. cmd->end()
11. graphicsQueue.submit(cmd)           -- submit to GPU
12. surface->endFrame(frame)            -- present to screen
```

## Cleanup order

Resources must be released in reverse creation order to avoid dangling
references:

1. `device->waitIdle()`
2. Mesh buffers (`mesh.cleanup()`)
3. Resource sets (`set->cleanup()`)
4. Pipeline objects (`pipeline->cleanup()`, `computePipeline->cleanup()`)
5. Pipeline layouts (`graphicsLayout->cleanup()`, `computeLayout->cleanup()`)
6. Samplers and images (reset `shared_ptr`)
7. Shader modules (`vs->cleanup()`, `fs->cleanup()`, `cs->cleanup()`)
8. `surface->cleanup()`
9. `device->cleanup()`
10. `instance->cleanup()`
11. `SDL_DestroyWindow()` / `SDL_Quit()` (if applicable)

## Physical device selection

When `PhysicalDevice::choose()` is called, the backend enumerates all available
GPUs and selects the one with the highest score. The scoring algorithm in
`PhysicalDeviceProperties::getScore()` works as follows:

- **Base score** = `totalHeapMemoryMB`
- **Multiplier by type**:
  - Discrete GPU: x100
  - Integrated GPU: x10
  - Virtual GPU: x5
  - CPU: +1
- **Ray tracing bonus**: if all ray tracing features are available, multiply by
  100

This ensures discrete GPUs with ray tracing are preferred when available, while
still producing a valid selection on systems with only integrated graphics.

### Ray tracing capabilities

The `RayTracingCapabilities` struct reports which features are available on the
selected device:

| Field                   | Description                              |
|-------------------------|------------------------------------------|
| `available`             | General ray tracing support              |
| `rayTracingPipeline`    | Hardware ray tracing pipeline            |
| `rayQuery`              | Ray queries from raster shaders          |
| `accelerationStructure` | BVH acceleration structures              |
| `bufferDeviceAddress`   | Buffer device address support            |

The `fullSupported()` method returns `true` only when all flags are `true`.

## Backend-specific accessors

When you need to access raw API handles (e.g., for third-party libraries or
advanced usage), each backend class exposes non-virtual accessors:

| Class               | Vulkan accessor         | Metal accessor          |
|---------------------|-------------------------|-------------------------|
| `vk::Instance`      | `vkInstanceHnd()`       | --                      |
| `vk::PhysicalDevice`| `handle()`              | --                      |
| `vk::Device`        | `handle()`              | --                      |
| `vk::WindowSurface` | `handle()`, `sdlWindow()`| --                     |
| `vk::Queue`         | `handle()`              | --                      |
| `vk::ShaderModule`  | `handle()`, `entryPoint()`| --                    |
| `vk::PipelineLayout`| `handle()`              | --                      |
| `vk::GraphicsPipeline`| `handle()`, `bindPoint()`| --                   |
| `vk::ComputePipeline`| `handle()`, `bindPoint()`| --                    |
| `vk::CommandBuffer` | `handle()`              | --                      |
| `vk::SurfaceFrame`  | `imageIndex()`, `imageAvailable()`, `renderFinished()`| --|
| `vk::Image`         | `handle()`, `imageView()`| --                     |
| `vk::Buffer`        | `handle()`              | --                      |
| `metal::PhysicalDevice`| --                    | `metalDevice()`         |
| `metal::Device`     | --                      | `handle()`              |
| `metal::WindowSurface`| --                     | `metalLayer()`          |
| `metal::Queue`      | --                      | `handle()`              |
| `metal::CommandBuffer`| --                     | `handle()`              |
| `metal::Image`      | --                      | `handle()`              |
| `metal::Buffer`     | --                      | `handle()`              |

To use these, cast the abstract pointer to the concrete type:

```cpp
auto* vkDevice = static_cast<gpu::vk::Device*>(device.get());
VkDevice vkDev = vkDevice->handle();
```
