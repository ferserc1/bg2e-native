# Tutorial 02: GPU Device Initialization

This tutorial walks through the `02_device` example: creating an SDL window, selecting a physical GPU, querying its properties, and creating a logical device with command queues. This is the essential setup before any rendering can happen.

**Source:** `examples/gpu/02_device/src/main.cpp`

## What you will learn

- How to select the GPU backend per platform
- How to create an SDL window with the correct flags for the chosen backend
- How to initialize the GPU instance and attach it to a window
- How to create a window surface and select a physical device
- How to query GPU properties (name, type, memory, ray tracing support)
- How to create a logical device and access command queues
- The correct cleanup order to avoid dangling references

## Prerequisites

- Completed [01_instance](01_instance.md) -- you should understand the Factory, Backend, and Instance concepts
- bg2e-native built and available on your system
- A Vulkan SDK installation (or MoltenVK on macOS)

## The object creation flow

This example exercises the full initialization pipeline of the GPU abstraction layer. The creation order matters -- each object depends on the ones created before it:

```
Factory::init(BackendType)
  -> Backend* backend = Factory::backend()
    -> backend->sharedInstance()                   // singleton Instance
      -> instance->create(window)                  // attach to SDL window
    -> backend->createWindowSurface(instance)      // rendering surface
    -> backend->createPhysicalDevice()             // GPU selector
      -> physicalDevice->choose(*instance, *surface)
    -> backend->createDevice()                     // logical device
      -> device->create(instance, physicalDevice, surface)
```

See [doc/api/gpu/index.md](../../api/gpu/index.md) for the full class hierarchy and creation flow.

## Step-by-step code explanation

### 1. Including required headers

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <iostream>
```

Three headers are needed:

| Header | Purpose |
|--------|---------|
| `<bg2e.hpp>` | The engine's umbrella header -- includes all namespaces |
| `<bg2e/gpu/all.hpp>` | The GPU abstraction layer (Factory, Backend, Device, etc.) |
| `<bg2e/app/SDLUtils.hpp>` | SDL helper utilities (`app::initSdlVideoDriver()`) |

The `<bg2e.hpp>` header already includes `<bg2e/gpu/all.hpp>`, so the second include is technically redundant. However, explicitly including `<bg2e/gpu/all.hpp>` makes the GPU dependency clear and is the recommended practice for GPU-focused programs.

### 2. Backend selection with user input

```cpp
auto backendType = gpu::BackendType::Vulkan;
if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
{
    std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
    int choice = 0;
    std::cin >> choice;
    backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
}
```

The engine supports two backends:

| Backend | `BackendType` value | Available on |
|---------|---------------------|--------------|
| Vulkan | `gpu::BackendType::Vulkan` | All platforms |
| Metal | `gpu::BackendType::Metal` | macOS only |

On Linux and Windows, Vulkan is the only option. On macOS, both Vulkan (via MoltenVK) and Metal are available, so this example lets the user choose at runtime. The `base::PlatformTools::currentPlatform()` utility detects the OS at compile time.

### 3. Initializing the backend factory

```cpp
gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();
```

`Factory::init()` creates the concrete `Backend` implementation (e.g., `gpu::vk::Backend` or `gpu::metal::Backend`) and stores it as a static singleton. `Factory::backend()` retrieves that singleton.

After this call the GPU abstraction layer is ready to create objects. The `Factory` class is entirely static -- you never instantiate it.

**Reference:** [Factory API](../../api/gpu/Factory.md)

### 4. SDL window creation with proper flags

```cpp
app::initSdlVideoDriver();
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
    800, 600,
    windowFlags
);
if (!window)
{
    std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
    return 1;
}
```

The window must be created with the correct SDL flags for the chosen backend:

- **Vulkan** requires `SDL_WINDOW_VULKAN` -- this tells SDL to load the Vulkan loader and create a `VkSurfaceKHR` internally.
- **Metal** requires `SDL_WINDOW_METAL` -- this tells SDL to create a `CAMetalLayer` for the window.

`backend->windowType()` returns a `WindowType` enum that maps directly to the required SDL flag. This is the bridge between the abstract GPU layer and the platform windowing system.

`app::initSdlVideoDriver()` is a bg2e utility that initializes the SDL video subsystem in a platform-aware way (e.g., handling Wayland vs X11 on Linux).

### 5. GPU instance creation and debug mode

```cpp
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create(window);
```

Three operations happen here:

1. **`sharedInstance()`** -- retrieves the singleton `Instance` object from the backend. This instance is owned by the backend; you never delete it.

2. **`enableDebugMode(true)`** -- enables validation layers (Vulkan) or debug instrumentation (Metal). This is essential during development to catch API misuse. In production you would disable it.

3. **`create(window)`** -- attaches the instance to the SDL window and initializes the graphics driver connection. The `window` parameter is required for windowed rendering; for headless use, call `instance->create()` with no arguments.

**Reference:** [Instance API](../../api/gpu/Instance.md)

### 6. Window surface creation

```cpp
auto surface = backend->createWindowSurface(instance);
```

A `Surface` represents the rendering target -- in this case, the SDL window's drawable area. The surface handles:

- Swapchain management (acquiring and presenting images)
- Present mode selection
- Surface format negotiation

The surface is created through the backend (not the instance) because it requires knowledge of both the window system and the graphics API.

**Reference:** [WindowSurface API](../../api/gpu/WindowSurface.md)

### 7. Physical device selection and properties

```cpp
auto physicalDevice = backend->createPhysicalDevice();
physicalDevice->choose(*instance, *surface);

auto props = physicalDevice->properties();
std::cout << "Selected GPU: " << props->name << std::endl;
std::cout << "  Type:        " << deviceTypeString(props->deviceType) << std::endl;
std::cout << "  Memory:      " << props->totalHeapMemoryMB << " MB" << std::endl;
std::cout << "  Ray Tracing: " << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;
```

`PhysicalDevice` wraps the physical GPU hardware. The `choose()` method:

1. Enumerates all available GPUs on the system
2. Scores each device using a weighted algorithm
3. Selects the one with the highest score

The scoring algorithm (from `PhysicalDeviceProperties::getScore()`) works as follows:

| Factor | Score calculation |
|--------|-------------------|
| **Base score** | `totalHeapMemoryMB` |
| **Device type multiplier** | Discrete GPU: x100, Integrated: x10, Virtual: x5, CPU: +1 |
| **Ray tracing bonus** | If all RT features available: multiply by 100 |

This ensures discrete GPUs with ray tracing are preferred when available, while still producing a valid selection on systems with only integrated graphics.

After `choose()`, `properties()` returns a `PhysicalDeviceProperties` struct with:

| Field | Description |
|-------|-------------|
| `name` | Human-readable GPU name (e.g., "NVIDIA GeForce RTX 4090") |
| `deviceType` | Enum: `DiscreteGPU`, `IntegratedGPU`, `VirtualGPU`, `CPU` |
| `totalHeapMemoryMB` | Total device-local memory in megabytes |
| `rayTracingSupported()` | `true` if all ray tracing features are available |

You can also call `PhysicalDevice::listSuitableDevices()` to enumerate all candidates before choosing one.

**Reference:** [PhysicalDevice API](../../api/gpu/PhysicalDevice.md), [PhysicalDeviceProperties API](../../api/gpu/PhysicalDeviceProperties.md)

### 8. Logical device creation and queue information

```cpp
auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());

std::cout << "  Graphics queue family: " << device->graphicsQueue().familyIndex() << std::endl;
std::cout << "  Present queue family:  " << device->presentQueue().familyIndex()  << std::endl;
std::cout << "  Transfer queue family: " << device->transferQueue().familyIndex() << std::endl;
std::cout << "  Swapchain images:      " << surface->imageCount() << std::endl;
```

The logical `Device` is the primary interface for all GPU operations. `device->create()` takes three parameters that were initialized in previous steps:

| Parameter | Purpose |
|-----------|---------|
| `instance` | The driver connection |
| `physicalDevice` | The selected GPU hardware |
| `surface` | The rendering target (needed to find compatible queue families) |

After creation, the device exposes three command queues:

| Queue | Purpose |
|-------|---------|
| `graphicsQueue()` | Graphics commands (rendering, compute) |
| `presentQueue()` | Presentation (submitting rendered images to the surface) |
| `transferQueue()` | Data transfers (buffer/image uploads) |

Each queue has a `familyIndex()` that identifies the hardware queue family it was created from. On many GPUs, graphics and present share the same family, but they can differ on multi-queue hardware.

The `surface->imageCount()` call returns the number of swapchain images (typically 2 or 3, depending on the presentation mode).

**Reference:** [Device API](../../api/gpu/Device.md), [Queue API](../../api/gpu/Queue.md)

### 9. Cleanup in reverse order

```cpp
device->waitIdle();
surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

Resources must be released in **reverse creation order** to avoid dangling references. The surface depends on the device, so it must be cleaned up first. The device depends on the instance, so it comes next.

| Step | Action | Reason |
|------|--------|--------|
| 1 | `device->waitIdle()` | Blocks until all pending GPU operations complete |
| 2 | `surface->cleanup()` | Destroys the swapchain and surface resources |
| 3 | `device->cleanup()` | Destroys the logical device and queues |
| 4 | `instance->cleanup()` | Destroys the driver connection and validation layers |
| 5 | `SDL_DestroyWindow()` | Destroys the OS window |
| 6 | `SDL_Quit()` | Shuts down SDL |

This example is console-only (it queries device info and exits), so there is no render event loop. Under Wayland, the window may never appear on screen because no buffer is ever committed -- this is expected behavior for a device-query program.

**Reference:** [Cleanup order](../../api/gpu/index.md#cleanup-order)

## Key concepts

### The initialization pipeline

The GPU abstraction layer enforces a strict initialization order. Each object depends on previously created objects:

```
Factory          (no dependencies)
  Backend        (created by Factory::init)
    Instance     (depends on Backend)
    Surface      (depends on Instance + Window)
    PhysicalDevice (depends on Instance + Surface)
    Device       (depends on Instance + PhysicalDevice + Surface)
```

This pipeline ensures that each object has access to the information it needs. For example, the physical device needs the surface to check presentation support, and the device needs the surface to find compatible queue families.

### Window type bridging

The `Backend::windowType()` method bridges the abstract GPU layer and the platform windowing system. SDL requires specific window flags for Vulkan (`SDL_WINDOW_VULKAN`) and Metal (`SDL_WINDOW_METAL`). The `WindowType` enum makes this mapping explicit and type-safe.

### Physical device auto-selection

The `PhysicalDevice::choose()` method abstracts away the complexity of GPU selection. The scoring algorithm handles common scenarios:

- Systems with both discrete and integrated GPUs prefer the discrete one
- Ray tracing support is heavily weighted (x100 bonus)
- Memory size is used as a tiebreaker

If you need manual control, use `PhysicalDevice::listSuitableDevices()` to enumerate all candidates and select one yourself.

### Cleanup order matters

Releasing resources in the wrong order can cause crashes or validation errors. The rule is simple: **destroy in reverse creation order**. The `CleanupManager` class (not used in this example) automates this with ordered and deferred cleanup.

## Complete source

For reference, here is the full source of `examples/gpu/02_device/src/main.cpp`:

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <iostream>

static const char* deviceTypeString(bg2e::gpu::PhysicalDeviceProperties::DeviceType type)
{
    switch (type)
    {
        case bg2e::gpu::PhysicalDeviceProperties::DiscreteGPU:   return "Discrete GPU";
        case bg2e::gpu::PhysicalDeviceProperties::IntegratedGPU: return "Integrated GPU";
        case bg2e::gpu::PhysicalDeviceProperties::VirtualGPU:    return "Virtual GPU";
        case bg2e::gpu::PhysicalDeviceProperties::CPU:           return "CPU";
        default:                                                  return "Unknown";
    }
}

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend to query window type
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. SDL init + create window based on backend window type
    app::initSdlVideoDriver();
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
        800, 600,
        windowFlags
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 4. Create GPU instance
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    // 5. Create surface
    auto surface = backend->createWindowSurface(instance);

    // 6. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    auto props = physicalDevice->properties();
    std::cout << "Selected GPU: " << props->name << std::endl;
    std::cout << "  Type:        " << deviceTypeString(props->deviceType) << std::endl;
    std::cout << "  Memory:      " << props->totalHeapMemoryMB << " MB" << std::endl;
    std::cout << "  Ray Tracing: " << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    std::cout << "  Graphics queue family: " << device->graphicsQueue().familyIndex() << std::endl;
    std::cout << "  Present queue family:  " << device->presentQueue().familyIndex()  << std::endl;
    std::cout << "  Transfer queue family: " << device->transferQueue().familyIndex() << std::endl;
    std::cout << "  Swapchain images:      " << surface->imageCount() << std::endl;

    // 8. Cleanup (reverse order)
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```

## Building and running

This example requires no shaders. Build it with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_device
```

Then run the binary from the `bin/` directory. On macOS you will be prompted to choose a backend. You should see output similar to:

```
Select backend [1=Metal, 2=Vulkan]: 2
Selected GPU: NVIDIA GeForce RTX 4090
  Type:        Discrete GPU
  Memory:      24564 MB
  Ray Tracing: Yes
  Graphics queue family: 0
  Present queue family:  0
  Transfer queue family: 1
  Swapchain images:      3
```

## Next steps

- **[03_offscreen_device](03_offscreen_device.md)** -- Initialize the GPU without a window for headless/offscreen rendering.
- **[05_simple_triangle](05_simple_triangle.md)** -- Render a triangle using the device you just learned to create.
- **[GPU API Reference](../../api/gpu/index.md)** -- Full class hierarchy, object creation flow, and backend-specific accessors.
