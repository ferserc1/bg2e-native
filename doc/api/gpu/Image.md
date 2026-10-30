# Image

**Header:** `<bg2e/gpu/Image.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Image {
public:
    virtual ~Image() = default;

    virtual void cleanup() = 0;
    virtual bool isValid() const = 0;

    PixelFormat   pixelFormat() const;
    const Size2D& size()        const;
    uint32_t      width()       const;
    uint32_t      height()      const;

    ImageLayout   currentLayout() const;
};
```

Abstract GPU image. Represents a texture or render target. Images are obtained
from `SurfaceFrame::colorImage()`, `SurfaceFrame::depthImage()`, or created
through `Device::createImage(ImageDescription)` or internally by the surface.

---

## Methods

### `virtual void cleanup() = 0`

Releases the image resources.

### `virtual bool isValid() const = 0`

Returns `true` if the image is valid and usable.

### `PixelFormat pixelFormat() const`

Returns the pixel format of the image.

### `const Size2D& size() const`

Returns the image dimensions as a `Size2D`.

### `uint32_t width() const`

Returns the image width in pixels.

### `uint32_t height() const`

Returns the image height in pixels.

### `ImageLayout currentLayout() const`

Returns the current uniform layout state of the image; mixed subresource layouts
throw. Use `currentLayout(range)` to inspect a selected range. Updated when
`CommandBuffer::transition()` is recorded.

---

## Example

```cpp
auto frame = surface->beginFrame();
auto* colorImg = frame->colorImage();

// Transition to color attachment
cmd->transition(colorImg, gpu::ImageLayout::ColorAttachment);
// colorImg->currentLayout() is now ImageLayout::ColorAttachment

// For a window presentation target, transition to present
cmd->transition(colorImg, gpu::ImageLayout::Present);
```

---

## vk::Image

**Header:** `<bg2e/gpu/vk/Image.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Image`

Vulkan image wrapper. Stores `VkImage`, `VkImageView`, and `VmaAllocation`.

### Vulkan-specific methods

| Method                | Return type    | Description                        |
|-----------------------|----------------|------------------------------------|
| `handle()`            | `VkImage`      | Raw Vulkan image handle.           |
| `imageView()`         | `VkImageView`  | Default image view.                |
| `allocation()`        | `VmaAllocation`| VMA memory allocation.             |

---

## metal::Image

**Header:** `<bg2e/gpu/metal/Image.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Image`

Metal image wrapper. Wraps `MTL::Texture*`.

### Metal-specific methods

| Method    | Return type     | Description                |
|-----------|-----------------|----------------------------|
| `handle()`| `MTL::Texture*` | Raw Metal texture handle.  |

See [rendering/subresources and transfer](Subresources_and_rendering.md) for
MRT attachments, explicit views/layouts, capability limits and synchronous IO.

## CPU image upload

`uploadImage(const base::Image*)` uploads matching 2D color dimensions. Four-channel
byte pixels targeting `R8G8B8A8_UNORM` and four-channel float pixels targeting
`R32G32B32A32_SFLOAT` are passed directly to the backend upload path, without an
intermediate conversion buffer or per-component float round trip. Other supported
layouts retain the generic channel/format conversion path.

This is not zero-copy GPU storage: backend staging allocation/copy and synchronous
submission still apply. Upload and mip generation are not batched. See
[image loading architecture](../../architecture/texture_and_image_load.md).

## Completed image readback

`readPixels(bytes, range, finalLayout)` is synchronous for one full initialized
color mip/layer with TransferSrc usage. It returns tightly packed native-format
bytes, with no row padding (including on Metal), and restores the specified final
layout; its default final layout is ShaderReadOnly. Call outside command recording
with producers stopped and keep the image/Device alive until it returns.

`readPixelsRGBA8(bytes, finalLayout)` accepts only R8G8B8A8_UNORM and selects the
base mip/layer. Despite the historical parameter name in the declaration, the
layout argument is the desired final layout. Pass TransferSrc explicitly for the
completed offscreen output. This helper supplies no BGRA, sRGB, HDR or row-orientation
conversion. Raw readPixels supports other color formats without encoding them.
`readPixelsDepth32(floats, finalLayout)` is the distinct initialized D32_SFLOAT
TransferSrc path; it is not a general packed depth/stencil decoder.

The [offscreen CLI](../../apps/draw_offscreen_application.md) selects RGBA8,
checks width×height×4 bytes, then uses DB PNG encode/decode to verify exact output
before publication. Color/alpha/orientation policy belongs to that caller, above
native GPU staging. See [subresources/transfer](Subresources_and_rendering.md).
