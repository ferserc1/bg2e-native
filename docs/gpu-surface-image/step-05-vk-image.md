# Step 5 — `gpu::Image` abstract base + `vk::Image`

**Goal:** Introduce the `gpu::Image` abstraction and its Vulkan implementation. `vk::Image`
owns a `VkImage` + main `VkImageView` + `VmaAllocation`, and supports:
- `buildTargetImage` — allocate a color render-target image.
- `buildDepthImage` — allocate a depth/stencil render-target image.
- `initFromSwapchainImage` — wrap an existing swapchain `VkImage` (creates a view, no allocation,
  does **not** own the `VkImage`).
- `resize` — destroy + recreate at a new size (only for allocated, not wrapped, images).
- `cleanup` — release everything it owns.

Not yet referenced by surfaces → build stays green.

## Files

- **Create** `lib/include/bg2e/gpu/Image.hpp`
- **Create** `lib/include/bg2e/gpu/vk/Image.hpp`, `lib/src/bg2e/gpu/vk/Image.cpp`
- **Modify** `lib/include/bg2e/gpu/all.hpp` (add `#include <bg2e/gpu/Image.hpp>`)

## `gpu/Image.hpp` (abstract)

```cpp
#pragma once
#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>   // Size2D, PixelFormat

namespace bg2e { namespace gpu {

class BG2E_API Image {
public:
    virtual ~Image() = default;

    virtual void cleanup() = 0;
    virtual bool isValid() const = 0;

    PixelFormat   pixelFormat() const { return _pixelFormat; }
    const Size2D& size()        const { return _size; }
    uint32_t      width()       const { return _size.width;  }
    uint32_t      height()      const { return _size.height; }

protected:
    PixelFormat _pixelFormat = PixelFormat::Undefined;
    Size2D      _size;
};

}}
```

> The build/wrap functions are **backend-specific** (they need native handles), so they live in
> the subclasses, not the abstract base. The base only standardizes the getters + lifecycle, as
> required.

## `vk/Image.hpp`

```cpp
#pragma once
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e { namespace gpu { namespace vk {

class Device;   // fwd

class Image : public gpu::Image {
public:
    Image() = default;
    ~Image() override { cleanup(); }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    // Allocate a color render-target image (usage: COLOR_ATTACHMENT | SAMPLED | TRANSFER_SRC/DST).
    void buildTargetImage(vk::Device* device, const Size2D& size, PixelFormat format);

    // Allocate a depth/stencil render-target image (usage: DEPTH_STENCIL_ATTACHMENT | SAMPLED).
    void buildDepthImage(vk::Device* device, const Size2D& size, PixelFormat format);

    // Wrap an existing swapchain image: creates the image view only, takes no ownership of the
    // VkImage and performs no allocation.
    void initFromSwapchainImage(vk::Device* device, VkImage image, PixelFormat format, const Size2D& size);

    // Destroy + recreate at a new size. Invalid for swapchain-wrapped images.
    void resize(const Size2D& size);

    void cleanup() override;
    bool isValid() const override { return _image != VK_NULL_HANDLE; }

    // Vulkan-specific accessors (used later for presentation / attachments).
    VkImage          handle()    const { return _image; }
    VkImageView      imageView() const { return _imageView; }
    VkFormat         vkFormat()  const { return _vkFormat; }
    VkImageAspectFlags aspect()  const { return _aspect; }

private:
    void createView();   // builds _imageView from _image/_vkFormat/_aspect

    vk::Device*   _device     = nullptr;       // not owned
    VkImage       _image      = VK_NULL_HANDLE;
    VkImageView   _imageView  = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE; // null for wrapped images
    VkFormat      _vkFormat   = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    bool          _ownsImage  = false;          // false for swapchain-wrapped
    VkImageUsageFlags _usage  = 0;              // remembered for resize
};

}}}
```

## `vk/Image.cpp` — behaviour

Includes: `<bg2e/gpu/vk/Image.hpp>`, `<bg2e/gpu/vk/Device.hpp>`, the format helpers from Step 2.

### `buildTargetImage(device, size, format)`
1. `cleanup()` any previous state. Store `_device`, `_size = size`, `_pixelFormat = format`,
   `_vkFormat = toVkFormat(format)`, `_aspect = VK_IMAGE_ASPECT_COLOR_BIT`,
   `_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT`, `_ownsImage = true`.
2. Fill `VkImageCreateInfo` (2D, mip=1, layers=1, samples=1, tiling OPTIMAL, initial layout
   UNDEFINED, extent `{size.width, size.height, 1}`).
3. `VmaAllocationCreateInfo` with `usage = VMA_MEMORY_USAGE_AUTO`,
   `requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`.
4. `VK_ASSERT(vmaCreateImage(device->allocator(), &imgInfo, &allocInfo, &_image, &_allocation, nullptr));`
5. `createView();`

### `buildDepthImage(device, size, format)`
Same as above but:
- `_aspect = VK_IMAGE_ASPECT_DEPTH_BIT` (add `| VK_IMAGE_ASPECT_STENCIL_BIT` when
  `hasStencil(format)`).
- `_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT`.

### `initFromSwapchainImage(device, image, format, size)`
1. `cleanup()`. Store `_device`, `_image = image`, `_size`, `_pixelFormat`,
   `_vkFormat = toVkFormat(format)`, `_aspect = COLOR`, `_allocation = VK_NULL_HANDLE`,
   `_ownsImage = false`.
2. `createView();`  (the view **is** owned and must be destroyed in cleanup).

### `createView()`
`VkImageViewCreateInfo` (2D, full subresource for `_aspect`, mip=1, layers=1) →
`VK_ASSERT(vkCreateImageView(device->handle(), &info, nullptr, &_imageView));`

### `resize(size)`
- If `!_ownsImage` → throw `std::runtime_error("vk::Image::resize: cannot resize a wrapped image")`.
- Remember `_device`, `_pixelFormat`, and whether it is depth (via `isDepthFormat`).
- `cleanup()` then call the matching `buildTargetImage`/`buildDepthImage` with the new size.

### `cleanup()`
```cpp
if (_imageView) vkDestroyImageView(_device->handle(), _imageView, nullptr);
if (_ownsImage && _image) vmaDestroyImage(_device->allocator(), _image, _allocation);
_imageView = VK_NULL_HANDLE; _image = VK_NULL_HANDLE; _allocation = VK_NULL_HANDLE;
// (guard each on _device != nullptr)
```
Wrapped images: only the view is destroyed; the `VkImage` belongs to the swapchain.

## `all.hpp`

Add `#include <bg2e/gpu/Image.hpp>` (after `Surface.hpp` line).

## Compile check

`vk::Image` compiles against the Step-4 `Device::allocator()` and Step-2 format helpers. No
surface references it yet. Build stays green. (Metal builds unaffected — no metal image yet.)
