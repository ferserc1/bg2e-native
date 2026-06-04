# WindowSurface

**Header:** `<bg2e/gpu/WindowSurface.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API WindowSurface : public Surface {
public:
    bool isOffscreen() const override { return false; }

    virtual void create(Instance* instance) = 0;
    virtual void cleanup() = 0;
};
```

Abstract surface backed by an OS window. Inherits from `Surface`.

---

## Methods

### `bool isOffscreen() const override`

Always returns `false`.

### `virtual void create(Instance* instance) = 0`

Initializes the surface using the given instance. The instance must already be
created in windowed mode.

| Parameter  | Type        | Description                          |
|------------|-------------|--------------------------------------|
| `instance` | `Instance*` | The GPU instance (must be windowed). |

### `virtual void cleanup() = 0`

Destroys the surface and releases associated resources.

---

## vk::WindowSurface

**Header:** `<bg2e/gpu/vk/WindowSurface.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::WindowSurface`

```cpp
class WindowSurface : public gpu::WindowSurface {
public:
    void create(gpu::Instance* instance) override;
    void cleanup() override;

    uint32_t width() const override;
    uint32_t height() const override;

    bool isValid() const override;

    VkSurfaceKHR handle() const;
    SDL_Window* sdlWindow() const;
};
```

Vulkan window surface (`VkSurfaceKHR`) backed by an SDL window.

### Vulkan-specific methods

#### `VkSurfaceKHR handle() const`

Returns the raw `VkSurfaceKHR` handle.

#### `SDL_Window* sdlWindow() const`

Returns the SDL window associated with this surface.

---

## metal::WindowSurface

**Header:** `<bg2e/gpu/metal/WindowSurface.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::WindowSurface`

```cpp
class WindowSurface : public gpu::WindowSurface {
public:
    void create(gpu::Instance* instance) override;
    void cleanup() override;

    uint32_t width() const override;
    uint32_t height() const override;
    bool isValid() const override;

    MetalLayerHandle metalLayer() const;
};
```

Metal window surface backed by a `CA::MetalLayer`. Uses an internal
`_metalView` (an `MTKView` or `CAMetalLayer`-bearing NSView).

### Metal-specific methods

#### `MetalLayerHandle metalLayer() const`

Returns the raw `CA::MetalLayer*` handle for use with Metal rendering.
