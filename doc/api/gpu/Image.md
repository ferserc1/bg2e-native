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
internally by the surface.

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

Returns the current layout state of the image. Updated automatically when
`CommandBuffer::transition()` is recorded.

---

## Example

```cpp
auto frame = surface->beginFrame();
auto* colorImg = frame->colorImage();

// Transition to color attachment
cmd->transition(colorImg, gpu::ImageLayout::ColorAttachment);
// colorImg->currentLayout() is now ImageLayout::ColorAttachment

// After rendering, transition to present
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
