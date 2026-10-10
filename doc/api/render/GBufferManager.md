# GBufferManager

**Header:** `<bg2e/render/gbuffer/GBufferManager.hpp>`
**Namespace:** `bg2e::render`

Owns configurable Vulkan color attachments and optional depth. The configurable
constructor supports the baker's depthless UV surface profile as well as ordinary
camera rendering.

## Configuration and lifecycle

`Configuration` contains `std::vector<VkFormat> colorFormats` and
`VkFormat depthFormat = VK_FORMAT_UNDEFINED`.

| Member | Contract |
|---|---|
| `GBufferManager(Engine*)` | Default deferred camera profile: six color images and D32 depth. |
| `GBufferManager(Engine*, const Configuration&)` | Copy formats; reject null engine and empty color-format list. Undefined depth omits depth allocation. |
| `build(VkExtent2D)` | Validate positive/device-supported dimensions, attachment count and format capabilities; replace images. |
| `resize(VkExtent2D)` | Rebuild at the new extent. |
| `cleanup()` | Release attachments and reset layout tracking. |
| destructor | Call cleanup; engine must remain alive. |

Callers must ensure old attachments are no longer in GPU use before rebuild or
cleanup. These methods do not themselves wait for device idle.

## Attachments and recording

| Signature | Contract |
|---|---|
| `uint32_t imageCount() const` | Number of color attachments. |
| `std::shared_ptr<vulkan::Image> image(uint32_t) const` | Color attachment; caller must supply an in-range index (no explicit bounds check). |
| `std::shared_ptr<vulkan::Image> depthImage() const` | Depth image or null for a depthless profile. |
| `const std::vector<const vulkan::Image*>& images() const` | Borrowed raw color pointers. |
| `const std::vector<VkFormat>& formats() const` | Color formats. |
| `VkFormat depthFormat() const` | Selected depth format. |
| `VkExtent2D extent() const` | Built dimensions. |
| `transitionToClear(VkCommandBuffer)` | Transition tracked images to GENERAL. |
| `transitionToAttachment(VkCommandBuffer)` | Transition color/depth for attachment use. |
| `transitionToShaderRead(VkCommandBuffer)` | Transition to shader-read layout, including later compute/RT reads. |
| `beginRender(VkCommandBuffer, bool isTransparent)` | Clear color and begin dynamic rendering with depth when configured; transparent depth load preserves existing depth. |
| `beginRender(VkCommandBuffer)` | Depthless overload; throws if a depth format is configured. Clear attachments and begin dynamic rendering. |

End dynamic rendering explicitly and transition attachments for subsequent readers.
Beginning before building throws `std::logic_error`; incompatible device limits
or format support are reported during build.

```cpp
bg2e::render::GBufferManager::Configuration profile;
profile.colorFormats = {
    VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_R32_UINT, VK_FORMAT_R32_UINT
};
bg2e::render::GBufferManager surface(engine, profile);
surface.build({512, 512});
surface.beginRender(cmd);
// Record the UV geometry draws.
bg2e::render::vulkan::cmdEndRendering(cmd);
surface.transitionToShaderRead(cmd);
```

Adapted from [UvSurfacePass](../../../lib/src/bg2e/render/UvSurfacePass.cpp).
Sources: [declaration](../../../lib/include/bg2e/render/gbuffer/GBufferManager.hpp),
[implementation](../../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp).
See [UV surface architecture](../../architecture/lightmaps/uv_surface_projection.md)
for attachment contents and projection semantics.
