# Phase 3 — GBuffer Manager

## Objective

Create the `GBufferManager` class that manages the creation, resizing, and lifecycle of G-buffer images. This class produces N images from a scene, serving as the foundation for deferred layer rendering.

---

## Important: Build Policy

**DO NOT compile or build the project.** The user will personally review and test each implementation step.

---

## Sub-phases

### 3.1 — Create GBufferManager Class

**Files:**
- `lib/include/bg2e/render/gbuffer/GBufferManager.hpp`
- `lib/src/bg2e/render/gbuffer/GBufferManager.cpp`

**Header structure:**
```
GBufferManager.hpp
├── GPL license header
├── #pragma once
├── Includes: Engine.hpp, vulkan/Image.hpp, common.hpp
├── Public:
│   ├── GBufferManager(Engine* engine)
│   ├── ~GBufferManager()
│   ├── build(VkExtent2D extent)
│   ├── resize(VkExtent2D newExtent)
│   ├── cleanup()
│   ├── imageCount() -> uint32_t
│   ├── image(uint32_t index) -> std::shared_ptr<vulkan::Image>
│   ├── depthImage() -> std::shared_ptr<vulkan::Image>
│   ├── images() -> const std::vector<const vulkan::Image*>&
│   ├── transitionToAttachment(VkCommandBuffer cmd)
│   ├── transitionToShaderRead(VkCommandBuffer cmd)
│   ├── clear(VkCommandBuffer cmd)
│   ├── formats() -> const std::vector<VkFormat>&
│   ├── depthFormat() -> VkFormat
│   └── extent() -> VkExtent2D
├── Protected:
│   ├── Engine* _engine
│   ├── VkExtent2D _extent
│   ├── std::vector<std::shared_ptr<vulkan::Image>> _colorImages
│   ├── std::vector<const vulkan::Image*> _colorImagePtrs
│   ├── std::vector<VkFormat> _colorFormats
│   ├── std::shared_ptr<vulkan::Image> _depthImage
│   └── VkFormat _depthFormat = VK_FORMAT_D32_SFLOAT
```

### 3.2 — G-Buffer Image Creation

Create 4 color images + 1 depth image in `build()`:

| Index | Format | Usage Flags | Aspect |
|-------|--------|-------------|--------|
| 0 (albedo) | `VK_FORMAT_R8G8B8A8_UNORM` | COLOR_ATTACHMENT \| SAMPLED \| TRANSFER_SRC \| TRANSFER_DST | COLOR |
| 1 (normals) | `VK_FORMAT_R8G8B8A8_SNORM` | COLOR_ATTACHMENT \| SAMPLED \| TRANSFER_SRC \| TRANSFER_DST | COLOR |
| 2 (materials) | `VK_FORMAT_R8G8B8A8_UNORM` | COLOR_ATTACHMENT \| SAMPLED \| TRANSFER_SRC \| TRANSFER_DST | COLOR |
| 3 (positions) | `VK_FORMAT_R32G32B32A32_SFLOAT` | COLOR_ATTACHMENT \| SAMPLED \| TRANSFER_SRC \| TRANSFER_DST | COLOR |
| depth | `VK_FORMAT_D32_SFLOAT` | DEPTH_STENCIL_ATTACHMENT \| SAMPLED | DEPTH |

All images are `VK_SAMPLE_COUNT_1_BIT` (no MSAA).

**Constructor:**
```cpp
GBufferManager::GBufferManager(Engine* engine)
    : _engine(engine)
{
    _colorFormats = {
        VK_FORMAT_R8G8B8A8_UNORM,        // albedo
        VK_FORMAT_R8G8B8A8_SNORM,        // normals
        VK_FORMAT_R8G8B8A8_UNORM,        // materials (metalness, roughness, ao, sheen)
        VK_FORMAT_R32G32B32A32_SFLOAT    // positions
    };
}
```

**`build()`:**
```cpp
void GBufferManager::build(VkExtent2D extent) {
    cleanup();
    _extent = extent;

    _colorImages.clear();
    _colorImagePtrs.clear();

    for (auto& format : _colorFormats) {
        auto image = vulkan::Image::createAllocatedImage(
            _engine, format, extent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,    // arrayLayers
            false, // useMipmaps
            0,    // maxMipmapLevels
            VK_SAMPLE_COUNT_1_BIT
        );
        _colorImages.push_back(image);
        _colorImagePtrs.push_back(image.get());
    }

    _depthImage = vulkan::Image::createAllocatedImage(
        _engine, _depthFormat, extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1, false, 0, VK_SAMPLE_COUNT_1_BIT
    );
}
```

### 3.3 — Image Access

**`image(uint32_t index)`:** Return `_colorImages[index]`.

**`depthImage()`:** Return `_depthImage`.

**`images()`:** Return `_colorImagePtrs` (raw pointer vector for passing to Vulkan APIs).

**`imageCount()`:** Return `_colorImages.size()`.

### 3.4 — Layout Transition Helpers

**`transitionToAttachment()`:**
```cpp
void GBufferManager::transitionToAttachment(VkCommandBuffer cmd) {
    for (auto& image : _colorImages) {
        vulkan::Image::cmdTransitionImage(cmd, image->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    vulkan::Image::cmdTransitionImage(cmd, _depthImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}
```

**`transitionToShaderRead()`:**
```cpp
void GBufferManager::transitionToShaderRead(VkCommandBuffer cmd) {
    for (auto& image : _colorImages) {
        vulkan::Image::cmdTransitionImage(cmd, image->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    vulkan::Image::cmdTransitionImage(cmd, _depthImage->handle(),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
```

### 3.5 — Clear Helper

**`clear()`:**
```cpp
void GBufferManager::clear(VkCommandBuffer cmd) {
    VkClearColorValue black{ { 0.0f, 0.0f, 0.0f, 0.0f } };
    for (auto& image : _colorImages) {
        auto range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, image->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &black, 1, &range);
    }

    VkClearDepthStencilValue depthClear{ 1.0f, 0 };
    auto depthRange = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
    vkCmdClearDepthStencilImage(cmd, _depthImage->handle(),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &depthClear, 1, &depthRange);
}
```

### 3.6 — Cleanup and Resize

**`cleanup()`:** Clear all vectors. Shared pointers release images automatically (VMA cleanup via `vulkan::Image` destructor).

**`resize()`:** Call `build()` with new extent (idempotent cleanup + recreate).

### 3.7 — Code Review Checklist

- [ ] `GBufferManager::build()` creates 4 color images + 1 depth image with correct formats
- [ ] All images use `VK_SAMPLE_COUNT_1_BIT` (no MSAA)
- [ ] Usage flags include `SAMPLED` (for compositing pass reading) and `TRANSFER_SRC/DST` (for potential copies)
- [ ] `transitionToAttachment()` transitions to correct layouts for rendering
- [ ] `transitionToShaderRead()` transitions to correct layouts for sampling
- [ ] `clear()` clears all images using `vkCmdClearColorImage` / `vkCmdClearDepthStencilImage`
- [ ] `resize()` recreates images at new extent
- [ ] `cleanup()` releases all Vulkan resources
- [ ] Header follows project conventions (GPL, pragma once, BG2E_API)

---

## Shader Development Notes

> **IMPORTANT:** No new shaders are needed in this phase. The GBufferManager only manages Vulkan images.

---

## Existing Code References

- `lib/include/bg2e/render/ColorAttachments.hpp` — similar image management pattern (vector of images with formats)
- `lib/src/bg2e/render/ColorAttachments.cpp` — image creation with `createAllocatedImage()`
- `lib/include/bg2e/render/vulkan/Image.hpp` — `createAllocatedImage()`, `cmdTransitionImage()`, `subresourceRange()`
