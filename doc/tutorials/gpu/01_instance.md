# Tutorial 01: GPU Instance Initialization

This tutorial walks through the first GPU example in the bg2e-native engine: creating a GPU backend instance. This is the foundational step before any rendering can happen.

**Source:** `examples/gpu/01_instance/src/main.cpp`

## What you will learn

- How the `bg2e::gpu` abstraction layer is organized
- How to select and initialize a graphics backend (Vulkan or Metal)
- How to obtain the shared GPU instance

## Prerequisites

- bg2e-native built and available on your system
- A Vulkan SDK installation (or MoltenVK on macOS)

## The GPU abstraction architecture

The `bg2e::gpu` namespace provides a backend-agnostic interface for GPU operations. Instead of writing code directly against Vulkan or Metal, you program against abstract interfaces. A factory creates the concrete backend at runtime.

The architecture has three tiers:

| Tier | Description |
|------|-------------|
| **Abstract interfaces** | Pure virtual classes in `bg2e::gpu::*` that define the contract |
| **Concrete backends** | Implementations in `bg2e::gpu::vk::*` (Vulkan) and `bg2e::gpu::metal::*` (Metal) |
| **Factory** | `bg2e::gpu::Factory` -- the entry point that creates the active backend singleton |

The creation flow is:

```
Factory::init(BackendType)
  -> Backend* backend = Factory::backend()
    -> backend->sharedInstance()
```

See [doc/api/gpu/index.md](../api/gpu/index.md) for the full class hierarchy.

## Step-by-step code explanation

### 1. Including the engine header

```cpp
#include <bg2e.hpp>
```

This single header pulls in the entire engine, including `bg2e::gpu`, `bg2e::base`, and all other namespaces. For GPU-only programs, you could also include `<bg2e/gpu/all.hpp>` directly, but `<bg2e.hpp>` is the standard entry point.

### 2. Selecting the backend

```cpp
auto backendType = gpu::BackendType::Vulkan;
if (base::PlatformTools::currentPlatform() == bg2e::base::Platform::macOS)
{
    backendType = gpu::BackendType::Metal;
}
```

The engine supports two backends:

| Backend | `BackendType` value | Available on |
|---------|---------------------|--------------|
| Vulkan | `gpu::BackendType::Vulkan` | All platforms |
| Metal | `gpu::BackendType::Metal` | macOS only |

On Linux and Windows, Vulkan is the only option and this conditional is unnecessary. On macOS, this example defaults to Metal because the Metal backend is being actively developed. In production code you might let the user choose at runtime (as the `02_device` example does).

`base::PlatformTools::currentPlatform()` is a utility in the `bg2e::base` namespace that detects the current OS at compile time.

### 3. Initializing the factory

```cpp
gpu::Factory::init(backendType);
```

This is the critical first call. `Factory::init()` does two things:

1. Creates the concrete `Backend` implementation for the chosen backend type (e.g., `gpu::vk::Backend` or `gpu::metal::Backend`).
2. Stores it as a static singleton accessible via `Factory::backend()`.

After this call, the GPU abstraction layer is ready to create objects. The `Factory` class is entirely static -- you never instantiate it yourself.

**Reference:** [Factory API](../api/gpu/index.md) -- `Factory::init(BackendType)` is the entry point for the object creation flow.

### 4. Getting the shared instance

```cpp
auto instance = gpu::Factory::backend()->sharedInstance();
```

This retrieves the singleton `Instance` object from the backend. The `Instance` represents the connection to the graphics driver. It manages:

- Debug mode and validation layers
- Application name registration
- Window association (for windowed rendering)

The `sharedInstance()` method returns a raw pointer to a singleton owned by the backend -- you do not create or destroy it yourself.

At this point the instance exists but is not yet attached to a window or initialized for rendering. The typical next steps (covered in later examples) would be:

```cpp
instance->enableDebugMode(true);       // enable validation layers
instance->create(window);              // attach to an SDL window (windowed)
// or
instance->create();                    // headless mode (offscreen)
```

**Reference:** [Instance API](../api/gpu/index.md) -- the `Instance` class provides `create(SDL_Window*)` for windowed rendering and `create()` for headless/offscreen use.

### 5. Printing a confirmation

```cpp
std::cout << "Hello instance" << std::endl;
```

This confirms the program ran successfully. In a real application you would continue with surface creation, device selection, and the render loop.

## Key concepts

### The Abstract Factory pattern

The `gpu::Factory` uses the Abstract Factory pattern to decouple your code from any specific graphics API. Your application only interacts with `gpu::Backend`, `gpu::Instance`, `gpu::Device`, and other abstract types. The concrete Vulkan or Metal implementations are hidden behind these interfaces.

This means the same application code can target Vulkan on Linux and Metal on macOS without `#ifdef` blocks scattered throughout the rendering code.

### Backend selection is a compile-time decision with runtime flexibility

While the `BackendType` is passed as a runtime parameter, each backend is a separate shared library. Only the backends available on the target platform will be linked. On macOS, both Vulkan (via MoltenVK) and Metal are available. On Linux and Windows, only Vulkan is included.

### Singleton ownership

The `Instance` returned by `sharedInstance()` is owned by the `Backend`. You never `delete` it or wrap it in a `std::unique_ptr`. The backend manages its lifetime.

## Complete source

For reference, here is the full source of `examples/gpu/01_instance/src/main.cpp`:

```cpp
#include <bg2e.hpp>

int main(int argc, char ** argv) {
    using namespace bg2e;

    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == bg2e::base::Platform::macOS)
    {
        backendType = gpu::BackendType::Metal;
    }

    gpu::Factory::init(backendType);
    auto instance = gpu::Factory::backend()->sharedInstance();

    std::cout << "Hello instance" << std::endl;
}
```

## Building and running

This example requires no shaders. Build it with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_instance
```

Then run the binary from the `bin/` directory. You should see:

```
Hello instance
```

## Next steps

- **[02_device](02_device.md)** -- Create an SDL window, select a physical GPU device, query its properties, and run an event loop.
- **[03_offscreen_device](03_offscreen_device.md)** -- Initialize the GPU without a window for headless/offscreen rendering.
- **[GPU API Reference](../api/gpu/index.md)** -- Full class hierarchy, object creation flow, and backend-specific accessors.
