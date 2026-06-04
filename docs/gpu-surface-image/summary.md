# gpu::Surface & gpu::Image — Implementation Plan (Summary)

> Scope note: the `bg2e::gpu` namespace is a **new, self-contained framework**. This plan
> only takes into account designs *inside* `gpu/` and only modifies files *inside* `gpu/`
> (headers under `lib/include/bg2e/gpu/`, sources under `lib/src/bg2e/gpu/`, and the
> `examples/gpu/` examples that exercise the framework). No code outside `gpu/` is touched.
> Designs from `render/` (or any other layer) are intentionally ignored.

## Goal

Turn `gpu::Surface` into the engine's **render-target provider** and introduce a new
`gpu::Image` abstraction. After this work:

- `gpu::Surface` is the source of truth for render-target **size**, **pixel format(s)** and
  **image count** (1 for offscreen, 2+ for double/triple buffered windows).
- Each concrete surface stores the backend resources required to *exist* as a render target:
  - **vk windowed** → `VkSwapchainKHR` + color images (wrapped) + depth image.
  - **vk offscreen** → color image(s) + depth image (allocated via VMA).
  - **metal windowed** → `CAMetalLayer` (+ `SDL_MetalView`) + depth texture.
  - **metal offscreen** → color texture + depth texture.
- `gpu::Image` (and backend impls `vk::Image`, `metal::Image`) own the creation, resize
  (destroy + recreate) and release of GPU image resources, and expose **pixel format** and
  **size** getters. The Vulkan image additionally owns its main `VkImageView`.

> These classes contain **only** resource creation / resize / release code. No presentation,
> no image-clearing, no command recording — that comes later.

## Confirmed design decisions

1. **Two-phase surface lifecycle.** Creating the concrete surface instance (via
   `Backend::createWindowSurface()` / `createOffscreenSurface()`) does **not** create its GPU
   resources. A virtual `Surface::createRenderTarget(Device*, PhysicalDevice*)` performs the
   effective resource creation, and is **invoked from `Device::create()`** (which already
   receives the surface) passing `this` as the initialized device.
   - For windowed surfaces, `WindowSurface::create(Instance*)` still runs first to create the
     platform surface (`VkSurfaceKHR` / `CAMetalLayer`), because physical-device selection
     needs it. The swapchain / depth / wrapped images are created later in
     `createRenderTarget()`.
2. **VMA allocator owned by `gpu::vk::Device`** (created in `Device::create`, destroyed in
   `Device::cleanup`, exposed via an `allocator()` getter). `vk::Image` allocates through it.
3. **Depth on every surface.** All four concrete surfaces own a depth resource, with a
   selectable depth `PixelFormat`. `PixelFormat::Undefined` for the depth format skips it.
4. **`Size2D` migration.** `gpu` size APIs move to a new `Size2D` struct: `Surface` stores a
   `Size2D` and exposes `size()` (with `width()/height()` kept as thin inline accessors over
   it); `Backend::createOffscreenSurface` takes a `Size2D`; `gpu::Image` exposes `size()`.

## New / changed public API (overview)

```
// gpu/Common.hpp
struct Size2D { uint32_t width = 0, height = 0; ... };
struct Size3D { uint32_t width = 0, height = 0, depth = 1; ... };
enum class PixelFormat { Undefined, R8G8B8A8_UNORM, B8G8R8A8_UNORM, R8G8B8A8_SRGB,
                         B8G8R8A8_SRGB, R16G16B16A16_SFLOAT, R32G32B32A32_SFLOAT,
                         D32_SFLOAT, D24_UNORM_S8_UINT, D32_SFLOAT_S8_UINT, D16_UNORM };

// gpu/Image.hpp  (abstract)
class Image {
    virtual ~Image();
    virtual void cleanup() = 0;
    virtual bool isValid() const = 0;
    PixelFormat pixelFormat() const;   // stored in base
    const Size2D& size() const;        // stored in base
};

// gpu/Surface.hpp  (abstract additions)
virtual void createRenderTarget(Device*, PhysicalDevice*) = 0;
virtual void resize(const Size2D&) = 0;
virtual void releaseRenderTarget() = 0;
PixelFormat colorFormat() const;  PixelFormat depthFormat() const;
virtual uint32_t imageCount() const = 0;
virtual gpu::Image* colorImage(uint32_t index) const = 0;  // nullptr for metal-windowed
virtual gpu::Image* depthImage() const = 0;
const Size2D& size() const;  uint32_t width() const; uint32_t height() const;
```

## Incremental steps

Each step leaves the project compiling, with **no forward dependency** on later steps.

| # | File | What it adds | Compiles because |
|---|------|--------------|------------------|
| 1 | [step-01-common-types.md](step-01-common-types.md) | `Size2D`, `Size3D`, `PixelFormat` in `gpu/Common.hpp` | Pure additive types, used nowhere yet |
| 2 | [step-02-format-conversion.md](step-02-format-conversion.md) | `PixelFormat` ↔ backend format helpers (vk + metal) | Standalone helper functions, unused yet |
| 3 | [step-03-size2d-migration.md](step-03-size2d-migration.md) | Migrate existing `Surface` / `Backend` size APIs to `Size2D` | Behaviour-preserving refactor incl. examples |
| 4 | [step-04-vma-allocator.md](step-04-vma-allocator.md) | `VmaAllocator` in `gpu::vk::Device` (+ getter) | Allocator created but not yet consumed |
| 5 | [step-05-vk-image.md](step-05-vk-image.md) | `gpu::Image` base + `vk::Image` (build / wrap / resize) | New classes, not referenced by surfaces yet |
| 6 | [step-06-metal-image.md](step-06-metal-image.md) | `metal::Image` (depth + offscreen color textures) | New class, not referenced by surfaces yet |
| 7 | [step-07-surface-render-target.md](step-07-surface-render-target.md) | Surface render-target phase + `Device::create` hook + examples | Base virtuals introduced together with all impls |

> Step 7 is the one larger step: introducing the pure-virtual render-target API on the
> `Surface` base requires implementing it in all four concrete surfaces simultaneously to
> keep the build green while preserving the framework's pure-abstract interface style.

## Files created / modified (full list)

**Created**
- `lib/include/bg2e/gpu/Image.hpp`
- `lib/include/bg2e/gpu/vk/Image.hpp`, `lib/src/bg2e/gpu/vk/Image.cpp`
- `lib/include/bg2e/gpu/metal/Image.hpp`, `lib/src/bg2e/gpu/metal/Image.cpp`
- (optional) `lib/include/bg2e/gpu/vk/Format.hpp`, `lib/include/bg2e/gpu/metal/Format.hpp`
  (or fold helpers into the existing `common.hpp` files)

**Modified**
- `lib/include/bg2e/gpu/Common.hpp`
- `lib/include/bg2e/gpu/Surface.hpp`, `WindowSurface.hpp`, `OffscreenSurface.hpp`, `Backend.hpp`, `all.hpp`
- `lib/include/bg2e/gpu/vk/{Device,WindowSurface,OffscreenSurface}.hpp` + matching `.cpp`
- `lib/include/bg2e/gpu/metal/{Device,WindowSurface,OffscreenSurface}.hpp` + matching `.cpp`
- `examples/gpu/02_device/src/main.cpp`, `examples/gpu/03_offscreen_device/src/main.cpp`

No CMake edits required (auto-glob covers new files under `lib/` and `examples/`).
