# Backend

**Header:** `<bg2e/gpu/Backend.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Backend {
public:
    virtual ~Backend() = default;

    [[nodiscard]] virtual gpu::Instance* sharedInstance() const = 0;
    [[nodiscard]] virtual WindowType windowType() const = 0;

    [[nodiscard]] virtual std::unique_ptr<gpu::PhysicalDevice>
        createPhysicalDevice() const = 0;

    [[nodiscard]] virtual std::unique_ptr<gpu::Device>
        createDevice() const = 0;

    [[nodiscard]] virtual std::unique_ptr<gpu::WindowSurface>
        createWindowSurface(
            gpu::Instance* instance,
            PixelFormat colorFormat = PixelFormat::B8G8R8A8_UNORM,
            PixelFormat depthFormat = PixelFormat::D32_SFLOAT) const = 0;

    [[nodiscard]] virtual std::unique_ptr<gpu::OffscreenSurface>
        createOffscreenSurface(
            gpu::Instance* instance,
            const Size2D& size,
            PixelFormat colorFormat = PixelFormat::R8G8B8A8_UNORM,
            PixelFormat depthFormat = PixelFormat::D32_SFLOAT) const = 0;
};
```

Abstract factory for all GPU subsystem objects. Each backend (Vulkan, Metal)
provides a concrete implementation that creates backend-specific instances.
Application code obtains a `Backend*` via `Factory::backend()` and uses it
to create all rendering objects.

---

## Methods

### `virtual gpu::Instance* sharedInstance() const = 0`

Returns the backend's singleton `Instance`. The instance is created lazily on
first call and owned by the backend. Do not delete the returned pointer.

### `virtual WindowType windowType() const = 0`

Returns the `WindowType` enum value corresponding to this backend. Use it to
determine the correct SDL window creation flags:

| Backend | Returns           | SDL flag           |
|---------|-------------------|--------------------|
| Vulkan  | `WindowType::Vulkan` | `SDL_WINDOW_VULKAN` |
| Metal   | `WindowType::Metal`  | `SDL_WINDOW_METAL`  |

### `virtual std::unique_ptr<gpu::PhysicalDevice> createPhysicalDevice() const = 0`

Creates a new `PhysicalDevice` object. The caller owns the returned pointer.
After creation, call `choose()` to select a GPU.

### `virtual std::unique_ptr<gpu::Device> createDevice() const = 0`

Creates a new `Device` object. The caller owns the returned pointer. After
creation, call `create()` to initialize the logical device.

### `virtual std::unique_ptr<gpu::WindowSurface> createWindowSurface(gpu::Instance* instance, PixelFormat colorFormat, PixelFormat depthFormat) const = 0`

Creates a new `WindowSurface` for rendering into an OS window. The surface is
initialized internally (no separate `create()` call needed).

| Parameter     | Type          | Default                  | Description                     |
|---------------|---------------|--------------------------|---------------------------------|
| `instance`    | `Instance*`   | --                       | The GPU instance (required).    |
| `colorFormat` | `PixelFormat` | `B8G8R8A8_UNORM`        | Color attachment pixel format.  |
| `depthFormat` | `PixelFormat` | `D32_SFLOAT`            | Depth attachment pixel format.  |

### `virtual std::unique_ptr<gpu::OffscreenSurface> createOffscreenSurface(gpu::Instance* instance, const Size2D& size, PixelFormat colorFormat, PixelFormat depthFormat) const = 0`

Creates a new `OffscreenSurface` for headless rendering.

| Parameter     | Type          | Default                  | Description                              |
|---------------|---------------|--------------------------|------------------------------------------|
| `instance`    | `Instance*`   | --                       | The GPU instance (required).             |
| `size`        | `Size2D`      | --                       | Width and height of the offscreen target.|
| `colorFormat` | `PixelFormat` | `R8G8B8A8_UNORM`        | Color attachment pixel format.           |
| `depthFormat` | `PixelFormat` | `D32_SFLOAT`            | Depth attachment pixel format.           |

---

## Example

```cpp
auto* backend = gpu::Factory::backend();

// Query window type for SDL
Uint32 flags = (backend->windowType() == gpu::WindowType::Vulkan)
    ? SDL_WINDOW_VULKAN : SDL_WINDOW_METAL;

// Create all subsystem objects
auto* instance = backend->sharedInstance();
auto surface = backend->createWindowSurface(instance);
auto physicalDevice = backend->createPhysicalDevice();
auto device = backend->createDevice();
```
