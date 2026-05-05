# Phase 1 — GpuAttachmentBuffer (G-Buffer Infrastructure)

## Objective

Create a centralized class that manages:
- Vulkan image creation for G-buffers (formats, usage flags, MSAA).
- Image resizing when the swapchain changes size.
- Lifecycle management (clean destruction).
- Layout transition and MSAA resolution helpers.

Class inspired by `ColorAttachments` but with full MSAA and depth attachment support.

---

## File to Create: `lib/include/bg2e/render/GpuAttachmentBuffer.hpp`

```
GpuAttachmentBuffer.hpp
├── license header (GPL)
├── #pragma once
├── Depends: Engine.hpp, vulkan/Image.hpp (existing includes)
├── Public
│   ├── GpuAttachmentBuffer(Engine* engine, VkExtent2D extent) constructor
│   ├── ~GpuAttachmentBuffer() destructor → cleanup()
│   ├── build(VkExtent2D extent) → creates all G-buffer images
│   ├── cleanup() → destroys Vulkan resources of G-buffers
│   └── imageWithIndex(uint32_t index) → shared_ptr<vulkan::Image>
│   └── [property getters] formats(), extent(), size(), msaaCount()
├── Protected / Private (methods for creating images)
│   ├── _createColorAttachments() → G-buffer color attachments (3) with MSAA 4x
│   ├── _createDepthAttachment() → depth buffer with MSAA (same sample count as colors)
│   └── _createResolveImages() → single-sample resolve targets (one per color attachment)
├── Protected members
│   ├── Engine* _engine
│   ├── VkExtent2D _extent
│   ├── std::vector<std::shared_ptr<vulkan::Image>> _gbufferColors → G-buffer MSAA color images
│   ├── std::vector<std::shared_ptr<vulkan::Image>> _resolveTargets → single-sample resolve targets
│   ├── std::shared_ptr<vulkan::Image> _depthBuffer → MSAA depth
│   ├── VkSampleCountFlagBits _msaaSampleCount
│   └── std::vector<VkFormat> _formats (3+1: 3 colors + depth)
```

### Detailed Format Decisions

| Attachment | Index | Format | Bits per pixel | Final Content |
|------------|-------|--------|----------------|---------------|
| Color 0 (albedo) | 0 | `VK_FORMAT_R8G8B8A8_UNORM` | 32 | Base color (no SRGB encoding) |
| Color 1 (normals) | 1 | `VK_FORMAT_R8G8B8A8_SNORM` | 32 | Tangent-space normals (normal.x = (tex.r * 2) - 1) |
| Color 2 (materials) | 2 | `VK_FORMAT_R8G8B8A8_UNORM` | 32 | Metalness(R), Roughness(G), AO(B), Emissive(A) |
| Depth prepass | — | `VK_FORMAT_D32_SFLOAT` | 32 | Single-sample depth for compositing occlusion (optional, see note below on depth) |

> **Note: single-sample vs MSAA in depth.**
> The G-buffer depth attachment must have the same sample count as the color attachments (dynamic rendering requires equal sample counts across all attachments). If we use MSAA 4x in color, depth must also be 4x.
> 
> **To avoid this**, we can use a hybrid approach: G-buffer depths in single-sample (depth prepass), and MSAA only on color attachments. This requires two draw calls per object: first a single-sample depth prepass, then a color pass with depth testing (but no writing) against that buffer.
> 
> **Adopted decision:** Use single-sample depth in the G-buffer pass (depth prepass), and MSAA only on color attachments. This is more efficient because:
> - The single-sample depth prepass is fast (only writes depth).
> - MSAA G-buffers improve quality at geometry edges.
> - No MSAA resolve of the depth buffer is needed (single sample).
> 
> To implement this, the draw must have two render-passes: one for depth-only (no color attachments), another for G-buffer colors with depth-test-read (no-write) against the depth prepass.

---

## File to Create: `lib/src/bg2e/render/GpuAttachmentBuffer.cpp`

### Constructor
```cpp
GpuAttachmentBuffer::GpuAttachmentBuffer(Engine* engine, VkExtent2D extent)
    : _engine(engine), _extent(extent), _msaaSampleCount(VK_SAMPLE_COUNT_4_BIT) 
{
    _formats = { VK_FORMAT_R8G8B8A8_UNORM, /* ... */ };
}
```

### build() — create all G-buffer images
- Call `cleanup()` (idempotent, clears already-deallocated resources).
- For each color attachment:
  - Call `vulkan::Image::createAllocatedImage()` with format, extent, usage flags (`COLOR_ATTACHMENT | SAMPLED | TRANSFER_DST | TRANSFER_SRC`), aspect `VK_IMAGE_ASPECT_COLOR_BIT`, sampleCount = `_msaaSampleCount`.
  - Store in `_gbufferColors`.
- Create depth attachment: single sample, `VK_FORMAT_D32_SFLOAT`, usage = `DEPTH_STENCIL_ATTACHMENT | SAMPLED`.
- Create resolve targets: one per color, single-sample, same format.

### cleanup() — destroy Vulkan resources
- Clear all `shared_ptr<VkImage>`. Vulkan destructions handled by vulkan::Image destructor (which uses VMA cleanup).

### imageWithIndex()
- Return `_gbufferColors[index]` (the MSAA image).

### Resolution Helpers
Add public method: `resolve(VkCommandBuffer cmd)` that blits each MSAA color attachment to its corresponding single-sample resolve target.

```cpp
void GpuAttachmentBuffer::resolve(VkCommandBuffer cmd) {
    for (size_t i = 0; i < _gbufferColors.size(); ++i) {
        vulkan::Image::cmdTransitionImage(
            cmd, _gbufferColors[i]->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );
        vulkan::Image::cmdTransitionImage(
            cmd, _resolveTargets[i]->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        // vkCmdCopyImage — blit MSAA resolve (linear filtering)
        _resolveTargets[i]->cmdCopy(_gbufferColors[i].get());  // reuse existing helper
        vulkan::Image::cmdTransitionImage(
            cmd, _gbufferColors[i]->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        vulkan::Image::cmdTransitionImage(
            cmd, _resolveTargets[i]->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    }
}
```

---

## Relationship with Existing `ColorAttachments` Class

| Feature | ColorAttachments (existing) | GpuAttachmentBuffer (new) |
|---------|----------------------------|--------------------------|
| Color attachments format vector | Yes (vector<VkFormat>) | Hardcoded in const array or std::array<format, 3> |
| MSAA support | No (single-sample) | Yes (4x on color attachments, single-sample on depth) |
| Depth buffer | No depth | Yes (single-sample depth prepass) |
| Resolve targets | No → G-buffer resolve in compositing | Yes (single-sample images for compositor reading) |
| Pipeline layout compatible | Uses only 1 color attachment | Can use up to 3+1 in dynamic rendering (VkPipelineRenderingCreateInfo already supports format vector) |

No modifications to `ColorAttachments` are needed (it already exists and is used by `ColorAttachmentsCanvas`). It is an independent class with distinct responsibilities.

---

## Phase 1 Checklist

- [ ] `GpuAttachmentBuffer::build()` creates all images with correct formats
- [ ] MSAA 4x applied to color attachments with correct resolution to single-sample targets
- [ ] Single-sample depth buffer for depth prepass
- [ ] `GpuAttachmentBuffer::resolve()` performs MSAA blit to resolve targets correctly
- [ ] Correct layout transitions: COLOR_ATTACHMENT_OPTIMAL → ... → SHADER_READ_ONLY_OPTIMAL
- [ ] `GpuAttachmentBuffer::cleanup()` releases all Vulkan/VMA resources correctly
