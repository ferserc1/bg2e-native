# Step 2 — `gpu::SurfaceFrame` (abstract + backend data holders)

**Goal:** introduce the per-frame render-target handle returned by `Surface::beginFrame()`. This
step only *defines* the types; nothing creates them yet (creation is wired in step 05). Build stays
green.

`SurfaceFrame` represents "the current frame / render target". Publicly it only needs to expose the
color and depth images; everything else (swapchain index, sync objects, drawable) is
backend-private data carried by the concrete subclasses.

## Files

- **New** `lib/include/bg2e/gpu/SurfaceFrame.hpp`
- **New** `lib/include/bg2e/gpu/vk/SurfaceFrame.hpp`
- **New** `lib/include/bg2e/gpu/metal/SurfaceFrame.hpp`
- **Modify** `lib/include/bg2e/gpu/all.hpp` (add the abstract header include)

> No `.cpp` files are required: the concrete `SurfaceFrame`s are trivial data holders with inline
> accessors. (A `.cpp` may be added in step 05 if any logic grows; auto-glob will pick it up.)

## `SurfaceFrame.hpp` (abstract)

```cpp
#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace gpu {

class Image;

class BG2E_API SurfaceFrame {
public:
    virtual ~SurfaceFrame() = default;

    virtual gpu::Image* colorImage() const = 0;
    virtual gpu::Image* depthImage() const = 0;

    virtual bool isValid() const = 0;
};

}
}
```

## `vk/SurfaceFrame.hpp`

Carries the Vulkan per-frame data the loop needs across `beginFrame` → `submit` → `present` →
`endFrame`. The color image points into the swapchain's `vk::Image` for the acquired index; the
depth image points into the surface-owned depth `vk::Image`. The synchronization objects
(`imageAvailable`, `renderFinished`, `inFlight`) and `imageIndex`/`swapchain` are read by
`vk::Queue::submit()` in step 05.

```cpp
#pragma once

#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Image;

class SurfaceFrame : public gpu::SurfaceFrame {
public:
    gpu::Image* colorImage() const override { return _colorImage; }
    gpu::Image* depthImage() const override { return _depthImage; }
    bool        isValid()    const override { return _colorImage != nullptr; }

    // --- set by WindowSurface::beginFrame() (step 05) ---
    void setColorImage(gpu::Image* img)        { _colorImage = img; }
    void setDepthImage(gpu::Image* img)        { _depthImage = img; }
    void setImageIndex(uint32_t index)         { _imageIndex = index; }
    void setSwapchain(VkSwapchainKHR sc)       { _swapchain = sc; }
    void setImageAvailable(VkSemaphore s)      { _imageAvailable = s; }
    void setRenderFinished(VkSemaphore s)      { _renderFinished = s; }
    void setInFlightFence(VkFence f)           { _inFlight = f; }

    uint32_t        imageIndex()     const { return _imageIndex; }
    VkSwapchainKHR  swapchain()      const { return _swapchain; }
    VkSemaphore     imageAvailable() const { return _imageAvailable; }
    VkSemaphore     renderFinished() const { return _renderFinished; }
    VkFence         inFlightFence()  const { return _inFlight; }

private:
    gpu::Image*    _colorImage     = nullptr;
    gpu::Image*    _depthImage     = nullptr;
    uint32_t       _imageIndex     = 0;
    VkSwapchainKHR _swapchain      = VK_NULL_HANDLE;
    VkSemaphore    _imageAvailable = VK_NULL_HANDLE;
    VkSemaphore    _renderFinished = VK_NULL_HANDLE;
    VkFence        _inFlight       = VK_NULL_HANDLE;
};

}
}
}
```

> Ownership: the `SurfaceFrame` does **not** own these objects. The sync objects and the depth/
> color `vk::Image`s are owned by `vk::WindowSurface` (created in `createRenderTarget`, destroyed
> in `releaseRenderTarget`). The frame is just a non-owning view valid for one iteration.

## `metal/SurfaceFrame.hpp`

The Metal frame wraps the `CA::MetalDrawable` acquired from the layer plus a transient color
`metal::Image` that wraps `drawable->texture()`, and points at the surface-owned depth image. All
Metal members must be guarded by `BG2E_IS_MAC` (the rest of the file compiles to an empty shell on
other platforms, matching the pattern already used by `metal::WindowSurface` / `metal::Image`).

```cpp
#pragma once

#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <memory>

namespace bg2e {
namespace gpu {
namespace metal {

class Image;

class SurfaceFrame : public gpu::SurfaceFrame {
public:
    gpu::Image* colorImage() const override;   // returns _colorImage.get()
    gpu::Image* depthImage() const override { return _depthImage; }
    bool        isValid()    const override;

#if BG2E_IS_MAC
    void setDrawable(CA::MetalDrawable* d)            { _drawable = d; }
    CA::MetalDrawable* drawable() const               { return _drawable; }
#endif

    void setColorImage(std::unique_ptr<metal::Image> img); // transient drawable-backed color
    void setDepthImage(gpu::Image* img) { _depthImage = img; }

private:
    std::unique_ptr<metal::Image> _colorImage;       // owns the drawable-texture wrapper
    gpu::Image*                   _depthImage = nullptr;
#if BG2E_IS_MAC
    CA::MetalDrawable*            _drawable   = nullptr;
#endif
};

}
}
}
```

`metal::Image` currently can only *create* textures it owns. It needs a non-owning wrap path for
the drawable texture (the Vulkan analogue, `initFromSwapchainImage`, already exists). Add to
`metal::Image` (header + `Image.cpp`, both `BG2E_IS_MAC` and stub branches):

```cpp
// metal/Image.hpp
void initFromDrawableTexture(metal::Device* device, TextureHandle texture,
                             PixelFormat format, const Size2D& size);
```

```cpp
// metal/Image.cpp  (BG2E_IS_MAC)
void Image::initFromDrawableTexture(metal::Device* device, TextureHandle texture,
                                    PixelFormat format, const Size2D& size)
{
    cleanup();
    _device      = device;
    _texture     = texture;     // NOT retained: owned by the drawable
    _size        = size;
    _pixelFormat = format;
    _isDepth     = false;
    _ownsTexture = false;       // new flag; cleanup() must not release when false
    _currentLayout = ImageLayout::Undefined;
}
```

Add a `bool _ownsTexture = true;` member to `metal::Image` and make `cleanup()` only call
`_texture->release()` when `_ownsTexture` is true. The existing `buildTargetImage` /
`buildDepthImage` set `_ownsTexture = true`. Provide a stub `initFromDrawableTexture` that throws
in the non-`BG2E_IS_MAC` branch (matching the file's existing pattern).

The trivial `colorImage()` / `isValid()` bodies for `metal::SurfaceFrame` go in a small
`metal/SurfaceFrame.cpp` (or inline) — both branches; on non-mac they can return `nullptr`/`false`.

## `all.hpp`

Add after the existing includes:

```cpp
#include <bg2e/gpu/SurfaceFrame.hpp>
```

(The backend-specific `vk/SurfaceFrame.hpp` and `metal/SurfaceFrame.hpp` are included by their
respective backend `.cpp` files, not by `all.hpp`, mirroring how `vk/WindowSurface.hpp` is kept out
of the public umbrella header.)

## Build check

Only new headers + small additive methods on `metal::Image`. Nothing instantiates a
`SurfaceFrame` yet. Build stays green on all platforms.
