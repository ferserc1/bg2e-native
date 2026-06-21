# Tutorial 03: Offscreen GPU Device Initialization

This tutorial walks through the `03_offscreen_device` example: initializing the GPU without a window for headless rendering. This is essential for server-side rendering, compute workloads, or any scenario where no OS window is needed.

**Source:** `examples/gpu/03_offscreen_device/src/main.cpp`

## What you will learn

- How to initialize the GPU without SDL or a window
- How to create an `OffscreenSurface` with fixed dimensions
- How the offscreen workflow differs from windowed rendering
- The complete GPU initialization pipeline for headless applications

## Prerequisites

- Completed [01_instance](01_instance.md) and [02_device](02_device.md)
- bg2e-native built and available on your system
- A Vulkan SDK installation (or MoltenVK on macOS)

## Understanding offscreen rendering

Offscreen rendering (also called headless rendering) allows you to use the GPU without creating an OS window. This is useful for:

- **Server-side rendering** -- generate images or videos without a display
- **Compute shaders** -- run GPU computations without visual output
- **Testing and CI** -- automated rendering tests without window system dependencies
- **Background processing** -- texture generation, post-processing, etc.

The key difference from windowed rendering is that you skip SDL entirely and use `OffscreenSurface` instead of `WindowSurface`. The rest of the GPU initialization pipeline remains identical.

**Reference:** [OffscreenSurface API](../../api/gpu/OffscreenSurface.md)

## The object creation flow

The offscreen workflow follows the same pattern as windowed rendering, but with two critical differences:

1. `instance->create()` is called without a window parameter
2. `backend->createOffscreenSurface()` replaces `backend->createWindowSurface()`

```
Factory::init(BackendType)
  -> Backend* backend = Factory::backend()
    -> backend->sharedInstance()                   // singleton Instance
      -> instance->create()                       // headless mode (no window)
    -> backend->createOffscreenSurface(instance, { 800, 600 })
    -> backend->createPhysicalDevice()
      -> physicalDevice->choose(*instance, *surface)
    -> backend->createDevice()
      -> device->create(instance, physicalDevice, surface)
```

**Reference:** [GPU API Index](../../api/gpu/index.md#offscreen-rendering)

## Step-by-step code explanation

### 1. Including required headers

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
#include <iostream>
```

Two headers are needed:

| Header | Purpose |
|--------|---------|
| `<bg2e/gpu/all.hpp>` | The GPU abstraction layer (Factory, Backend, Device, etc.) |
| `<bg2e/base/all.hpp>` | Platform utilities (`base::PlatformTools`) |

Note that this example does **not** include `<bg2e/app/SDLUtils.hpp>` because no SDL window is created. This is the first indication that this is an offscreen program.

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

**Reference:** [BackendType API](../../api/gpu/BackendType.md)

### 3. Initializing the backend factory

```cpp
gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();
```

`Factory::init()` creates the concrete `Backend` implementation (e.g., `gpu::vk::Backend` or `gpu::metal::Backend`) and stores it as a static singleton. `Factory::backend()` retrieves that singleton.

After this call the GPU abstraction layer is ready to create objects. The `Factory` class is entirely static -- you never instantiate it.

**Reference:** [Factory API](../../api/gpu/Factory.md)

### 4. Creating GPU instance without window

```cpp
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create();  // headless overload
```

Three operations happen here:

1. **`sharedInstance()`** -- retrieves the singleton `Instance` object from the backend. This instance is owned by the backend; you never delete it.

2. **`enableDebugMode(true)`** -- enables validation layers (Vulkan) or debug instrumentation (Metal). This is essential during development to catch API misuse. In production you would disable it.

3. **`create()`** -- creates the instance in headless/offscreen mode with **no window parameter**. This is the key difference from windowed rendering:
   - **Windowed:** `instance->create(window)` -- attaches to an SDL window
   - **Headless:** `instance->create()` -- no window association

The headless overload sets `presentationMode()` to `PresentationMode::Offscreen` and initializes the driver connection without any window system dependencies.

**Reference:** [Instance API](../../api/gpu/Instance.md)

### 5. Creating offscreen surface with fixed dimensions

```cpp
auto surface = backend->createOffscreenSurface(instance, gpu::Size2D{ 800, 600 });
```

This creates a rendering target with fixed dimensions (800x600 pixels). Unlike `WindowSurface`, which automatically negotiates formats with the display system, `OffscreenSurface` uses sensible defaults:

| Parameter | Default Value | Description |
|-----------|---------------|-------------|
| `colorFormat` | `R8G8B8A8_UNORM` | 32-bit RGBA color |
| `depthFormat` | `D32_SFLOAT` | 32-bit depth buffer |

The `Size2D` struct specifies the width and height in pixels. These dimensions are fixed for the lifetime of the surface (though you can call `resize()` later if needed).

**Key differences from WindowSurface:**

| Aspect | WindowSurface | OffscreenSurface |
|--------|---------------|------------------|
| **Creation** | Automatic via `createWindowSurface()` | Explicit with size parameter |
| **Size** | Determined by window | Fixed at creation |
| **Format** | Negotiated with display | Uses defaults |
| **Swapchain** | Multiple images (2-3) | Single image (`imageCount()` returns 1) |
| **Presentation** | To screen | To memory (read pixels) |

**Reference:** [OffscreenSurface API](../../api/gpu/OffscreenSurface.md), [Surface API](../../api/gpu/Surface.md)

### 6. Physical device selection and properties

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

### 7. Logical device creation and queue information

```cpp
auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());

std::cout << "  Graphics queue family: " << device->graphicsQueue().familyIndex() << std::endl;
std::cout << "  Present queue family:  " << device->presentQueue().familyIndex()  << std::endl;
std::cout << "  Transfer queue family: " << device->transferQueue().familyIndex() << std::endl;
std::cout << "  Image count:           " << surface->imageCount() << std::endl;
std::cout << "  Surface size:          " << surface->width() << "x" << surface->height() << std::endl;
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

The `surface->imageCount()` call returns `1` for offscreen surfaces (vs. 2-3 for window surfaces). The `surface->width()` and `surface->height()` return the dimensions specified in the constructor.

**Reference:** [Device API](../../api/gpu/Device.md), [Queue API](../../api/gpu/Queue.md)

### 8. Cleanup in reverse order

```cpp
device->waitIdle();
surface->cleanup();
device->cleanup();
instance->cleanup();
```

Resources must be released in **reverse creation order** to avoid dangling references. The surface depends on the device, so it must be cleaned up first. The device depends on the instance, so it comes next.

| Step | Action | Reason |
|------|--------|--------|
| 1 | `device->waitIdle()` | Blocks until all pending GPU operations complete |
| 2 | `surface->cleanup()` | Destroys the offscreen render target |
| 3 | `device->cleanup()` | Destroys the logical device and queues |
| 4 | `instance->cleanup()` | Destroys the driver connection and validation layers |

Note the absence of `SDL_DestroyWindow()` and `SDL_Quit()` -- there is no SDL window to clean up in this example.

**Reference:** [Cleanup order](../../api/gpu/index.md#cleanup-order)

## Key concepts

### Offscreen vs. windowed initialization

The initialization pipeline is nearly identical, with two critical differences:

| Step | Windowed | Offscreen |
|------|----------|-----------|
| **Instance creation** | `instance->create(window)` | `instance->create()` |
| **Surface creation** | `backend->createWindowSurface(instance)` | `backend->createOffscreenSurface(instance, size)` |
| **SDL dependency** | Required | Not required |
| **Window flags** | `SDL_WINDOW_VULKAN` or `SDL_WINDOW_METAL` | None |

### When to use offscreen rendering

Use offscreen rendering when:

- **No display is available** -- servers, containers, CI/CD pipelines
- **You need pixel readback** -- render to texture, capture screenshots
- **You're doing compute work** -- no visual output needed
- **You want to avoid window system dependencies** -- simpler testing, fewer platform-specific issues

### The OffscreenSurface lifecycle

Unlike `WindowSurface`, which manages a swapchain with multiple images, `OffscreenSurface` creates a single render target:

1. **Construction** -- specifies width and height
2. **Device creation** -- device uses the surface to find queue families
3. **Rendering** -- use `beginFrame()` / `present()` / `endFrame()` as usual
4. **Readback** -- access pixels via `colorImage()` for CPU readback
5. **Cleanup** -- release resources in reverse order

### Debug mode in offscreen applications

Debug mode is especially valuable in offscreen applications because:

- **No visual feedback** -- validation layers catch errors you can't see
- **Server-side crashes** -- validation errors become logs instead of silent failures
- **Performance profiling** -- GPU debug tools can analyze offscreen workloads

Always enable debug mode during development:

```cpp
instance->enableDebugMode(true);
```

## Complete source

For reference, here is the full source of `examples/gpu/03_offscreen_device/src/main.cpp`:

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
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

int main(int /*argc*/, char** /*argv*/)
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

    // 2. Init backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. Create GPU instance (offscreen — no window)
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create();

    // 4. Create offscreen surface
    auto surface = backend->createOffscreenSurface(instance, gpu::Size2D{ 800, 600 });

    // 5. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    auto props = physicalDevice->properties();
    std::cout << "Selected GPU: " << props->name << std::endl;
    std::cout << "  Type:        " << deviceTypeString(props->deviceType) << std::endl;
    std::cout << "  Memory:      " << props->totalHeapMemoryMB << " MB" << std::endl;
    std::cout << "  Ray Tracing: " << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;

    // 6. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    std::cout << "  Graphics queue family: " << device->graphicsQueue().familyIndex() << std::endl;
    std::cout << "  Present queue family:  " << device->presentQueue().familyIndex()  << std::endl;
    std::cout << "  Transfer queue family: " << device->transferQueue().familyIndex() << std::endl;
    std::cout << "  Image count:           " << surface->imageCount() << std::endl;
    std::cout << "  Surface size:          " << surface->width() << "x" << surface->height() << std::endl;

    // 7. Cleanup (reverse order; surface render target depends on device)
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();

    return 0;
}
```

## Building and running

This example requires no shaders. Build it with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_offscreen_device
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
  Image count:           1
  Surface size:          800x600
```

Note the `Image count: 1` -- this confirms we're using an offscreen surface with a single render target, not a swapchain with multiple images.

## Next steps

- **[05_simple_triangle](05_simple_triangle.md)** -- Render a triangle using the device you just learned to create.
- **[07_uniform_buffers](07_uniform_buffers.md)** -- Add per-frame uniform buffers for dynamic data.
- **[GPU API Reference](../../api/gpu/index.md)** -- Full class hierarchy, object creation flow, and backend-specific accessors.