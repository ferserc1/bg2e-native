# Step 005 — `vk::Image::readPixelsRGBA8`

**Type:** Backend (Vulkan)
**Depends on:** 002 (`gpu::Device::immediateSubmit`, Vulkan override), 004 (base API)
**Enables:** 007 (example, Vulkan path)

## Goal

Implement the Vulkan GPU→CPU read-back: copy the image into a host-visible VMA staging buffer
via the backend-agnostic `gpu::Device::immediateSubmit` + `vkCmdCopyImageToBuffer`, then map and
`memcpy` into the output vector. Mirrors `bg2e::render::vulkan::Image::readPixelsRGBA8` but uses
the new `gpu::vk` types and an inline VMA buffer (no `gpu::Buffer` abstraction).

The layout transitions are recorded through the **abstract** command buffer
(`cmd->transition(...)`); only the copy command needs the native handle, obtained by
down-casting the closure's `gpu::CommandBuffer*`.

## API (`lib/include/bg2e/gpu/vk/Image.hpp`)

```cpp
void readPixelsRGBA8(std::vector<uint8_t>& outData,
                     ImageLayout currentLayout = ImageLayout::ColorAttachment) override;
```

## Implementation (`lib/src/bg2e/gpu/vk/Image.cpp`)

```cpp
void Image::readPixelsRGBA8(std::vector<uint8_t>& outData, ImageLayout currentLayout)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM) {
        throw std::runtime_error("vk::Image::readPixelsRGBA8: only R8G8B8A8_UNORM is supported");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;
    const VkDeviceSize imageSize = VkDeviceSize(w) * h * 4;

    // 1. Inline host-visible staging buffer (GPU -> CPU).
    VkBuffer staging = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = imageSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
    allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT; // persistently mapped
    VmaAllocationInfo allocOut{};
    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                              &staging, &stagingAlloc, &allocOut));

    // The image's tracked layout must match `currentLayout` so the abstract transition below
    // emits the correct oldLayout. The caller passes the layout the image is currently in.
    setCurrentLayout(currentLayout);

    // 2. Synchronous copy via the backend-agnostic immediate submit.
    //    Transitions go through the ABSTRACT command buffer; only the copy needs the native handle.
    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        cmd->transition(this, ImageLayout::TransferSrc);  // current -> TransferSrc (abstract)

        VkCommandBuffer raw = dynamic_cast<vk::CommandBuffer*>(cmd)->handle();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;     // tightly packed
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { w, h, 1 };

        vkCmdCopyImageToBuffer(raw, _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               staging, 1, &region);

        cmd->transition(this, currentLayout);             // TransferSrc -> current (abstract)
    });

    // 3. Copy out (image is back in its original layout, tracked correctly by the transitions).
    outData.resize(imageSize);
    std::memcpy(outData.data(), allocOut.pMappedData, imageSize);

    vmaDestroyBuffer(_device->allocator(), staging, stagingAlloc);
}
```

## Notes / pitfalls

- **Transitions reuse the abstract API.** Because the closure receives a `gpu::CommandBuffer*`,
  the layout transitions are done with `cmd->transition(this, ...)` (the existing
  `vk::CommandBuffer::transition`, which already builds the `VkImageMemoryBarrier2` and updates
  the image's tracked layout). No custom `toVkLayout`/`cmdTransition` helper is needed — this is
  the main simplification from making `immediateSubmit` backend-agnostic.
- `this` is a `vk::Image*`, which is a `gpu::Image*`, so it binds directly to
  `transition(gpu::Image*, ImageLayout)`. The down-cast `dynamic_cast<vk::CommandBuffer*>(cmd)`
  is used **only** to reach the native `VkCommandBuffer` for `vkCmdCopyImageToBuffer`.
- The color target image is created with `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` already
  (`buildTargetImage`), so no usage change is needed.
- Includes needed in `Image.cpp`: `<bg2e/gpu/CommandBuffer.hpp>`,
  `<bg2e/gpu/vk/CommandBuffer.hpp>`, `<bg2e/gpu/vk/Device.hpp>` (already), and `<cstring>` for
  `memcpy`.

## Validation

- `bg2e` builds; existing examples unchanged.
- After step 007, the Vulkan run produces a correct `out.jpg` (triangle over clear color).
- No validation-layer errors (transitions are balanced; staging buffer destroyed after the
  blocking `immediateSubmit`).
