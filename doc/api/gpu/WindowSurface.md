# WindowSurface

**Header:** `<bg2e/gpu/WindowSurface.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API WindowSurface : public Surface {
public:
    bool isOffscreen() const override { return false; }
};
```

Abstract surface backed by an OS window. Inherits from `Surface`.

The `create(Instance*)` method is **protected** and called automatically by
`Backend::createWindowSurface()`. Application code does not call `create()`
directly.

---

## Methods

### `bool isOffscreen() const override`

Always returns `false`.

---

## vk::WindowSurface

**Header:** `<bg2e/gpu/vk/WindowSurface.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::WindowSurface`

```cpp
class WindowSurface : public gpu::WindowSurface {
public:
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
