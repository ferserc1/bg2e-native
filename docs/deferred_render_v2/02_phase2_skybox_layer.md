# Phase 2 — Skybox Layer

## Objective

Implement the skybox rendering as the first layer of the deferred pipeline. The skybox is rendered to an intermediate image, which is then copied to the swapchain output image. This establishes the layer pattern and the image management infrastructure.

---

## Important: Build Policy

**DO NOT compile or build the project.** The user will personally review and test each implementation step.

---

## Sub-phases

### 2.1 — Create SkyboxLayer Class

**Files:**
- `lib/include/bg2e/render/deferred/SkyboxLayer.hpp`
- `lib/src/bg2e/render/deferred/SkyboxLayer.cpp`

The `SkyboxLayer` is the simplest deferred layer. It does not use G-buffers. It renders the skybox to an output image.

**Header structure:**
```
SkyboxLayer.hpp
├── GPL license header
├── #pragma once
├── Includes: Engine.hpp, render/SkyboxRenderer.hpp, vulkan/Image.hpp, scene/Scene.hpp
├── Public:
│   ├── SkyboxLayer(Engine* engine)
│   ├── ~SkyboxLayer()
│   ├── build(VkExtent2D extent, VkFormat outputFormat, VkSampleCountFlagBits sampleCount)
│   ├── initFrameResources(vulkan::DescriptorSetAllocator* allocator)
│   ├── render(VkCommandBuffer cmd, uint32_t currentFrame,
│   │          const vulkan::Image* inputImage,     // nullptr for skybox
│   │          const vulkan::Image* outputImage,     // image to render skybox into
│   │          vulkan::FrameResources& frameResources,
│   │          scene::Scene* scene,
│   │          EnvironmentResources* environment)
│   ├── resize(VkExtent2D newExtent)
│   └── cleanup()
├── Protected:
│   ├── Engine* _engine
│   ├── VkExtent2D _extent
│   ├── VkFormat _outputFormat
│   └── std::unique_ptr<SkyboxRenderer> _skyboxRenderer
```

### 2.2 — SkyboxLayer Implementation

**Constructor:** Store engine pointer.

**`build()`:**
1. Store extent and output format.
2. Create `_skyboxRenderer = std::make_unique<SkyboxRenderer>(_engine)`.
3. Call `_skyboxRenderer->setSampleCount(sampleCount)`.
4. Call `_skyboxRenderer->build(skyTexture, {outputFormat}, VK_FORMAT_D32_SFLOAT)`.
   - Note: The skybox renderer needs a texture. The texture will be set when `render()` is called with an `EnvironmentResources*` that holds the skybox texture.
   - Alternative: Store the skybox texture reference and pass it during `build()`.

**`initFrameResources()`:** Delegate to `_skyboxRenderer->initFrameResources(allocator)`.

**`render()`:**
1. Clear the output image to black using `vulkan::macros::cmdClearImageAndBeginRendering()`.
2. Set viewport/scissor using `vulkan::macros::cmdSetDefaultViewportAndScissor()`.
3. Get camera view/projection matrices from `scene->mainCamera()`:
   ```cpp
   auto mainCamera = scene->mainCamera();
   auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
   auto projMatrix = mainCamera->projectionMatrix();
   ```
4. Set skybox color correction: `_skyboxRenderer->setBrightness(...)`, etc.
5. Update skybox matrices: `_skyboxRenderer->update(viewMatrix, projMatrix)`.
6. Draw skybox: `_skyboxRenderer->draw(cmd, currentFrame, frameResources)`.
7. End rendering: `vulkan::cmdEndRendering(cmd)`.

**`resize()`:** Update `_extent`.

**`cleanup()`:** Reset `_skyboxRenderer`.

### 2.3 — Integrate SkyboxLayer into RendererDeferred

Update `RendererDeferred` to:
1. Own an `EnvironmentResources` instance (for IBL and skybox texture)
2. Own a `SkyboxLayer` instance
3. Create an intermediate image in `build()`
4. In `draw()`: render the skybox to the intermediate image, then copy to swapchain output

**New members in `RendererDeferred`:**
```cpp
protected:
    std::unique_ptr<EnvironmentResources> _environment;
    std::unique_ptr<SkyboxLayer> _skyboxLayer;
    std::shared_ptr<vulkan::Image> _intermediateImage;
```

**`build()` additions:**
```cpp
_environment = std::make_unique<EnvironmentResources>(_engine);
// Build environment with default sky dome
_environment->build(/* default sky dome texture */);

_skyboxLayer = std::make_unique<SkyboxLayer>(_engine);
_skyboxLayer->build(initialExtent, colorImageFormat, VK_SAMPLE_COUNT_1_BIT);

_intermediateImage = vulkan::Image::createAllocatedImage(
    _engine, colorImageFormat, initialExtent,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT
);
```

**`draw()` implementation:**
```cpp
void RendererDeferred::draw(VkCommandBuffer cmd, uint32_t currentFrame,
                            const vulkan::Image* colorImage,
                            const vulkan::Image* depthImage,
                            const vulkan::Image* msaaDepthImage,
                            vulkan::FrameResources& frameResources) {
    _scene->willDraw();

    // Update environment (IBL, skybox texture if changed)
    _environment->update(cmd, currentFrame, frameResources);

    // === Layer 1: Skybox ===
    _skyboxLayer->render(cmd, currentFrame, nullptr, _intermediateImage.get(),
                         frameResources, _scene.get(), _environment.get());

    // === Copy intermediate to swapchain output ===
    vulkan::Image::cmdTransitionImage(cmd, _intermediateImage->handle(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vulkan::Image::cmdTransitionImage(cmd, colorImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vulkan::Image::cmdCopy(cmd, _intermediateImage.get(), _intermediateImage->extent(),
        colorImage.get(), colorImage->extent());
    vulkan::Image::cmdTransitionImage(cmd, colorImage->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    _scene->didDraw();
}
```

**`resize()` additions:**
```cpp
_skyboxLayer->resize(newExtent);
// Recreate intermediate image at new extent
_intermediateImage = vulkan::Image::createAllocatedImage(
    _engine, _colorImageFormat, newExtent, ...
);
```

**`cleanup()` additions:**
```cpp
_skyboxLayer->cleanup();
_environment->cleanup();
_intermediateImage.reset();
```

### 2.4 — Intermediate Image Management

The intermediate image is created with the same format as the swapchain color image. It serves as the render target for the skybox layer and will be reused in later phases as the input to the opaque layer.

**Creation:**
```cpp
_intermediateImage = vulkan::Image::createAllocatedImage(
    _engine, _colorImageFormat, _viewportExtent,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT
);
```

**Cleanup registration:** Register cleanup via `_engine->cleanupManager().push()` if needed (check if `vulkan::Image` destructor handles this automatically via VMA).

### 2.5 — Code Review Checklist

- [ ] `SkyboxLayer` class follows the project's header/source conventions
- [ ] `SkyboxRenderer` is reused correctly (no reinventing skybox rendering)
- [ ] Intermediate image is created with correct format and usage flags
- [ ] Image copy from intermediate to swapchain uses correct layout transitions
- [ ] Camera matrices are extracted correctly from `scene->mainCamera()`
- [ ] `resize()` recreates the intermediate image at new extent
- [ ] `cleanup()` releases all resources in correct order

---

## Shader Development Notes

> **IMPORTANT:** Any questions about shader implementation should be asked before proceeding. The skybox layer reuses the existing `skybox_renderer.vert.spv` and `skybox_renderer.frag.spv` shaders — no new shaders are needed in this phase. The existing shader library should be used for all operations.

---

## Existing Code References

- `lib/include/bg2e/render/SkyboxRenderer.hpp` — skybox renderer API
- `lib/src/bg2e/render/SkyboxRenderer.cpp` — skybox renderer implementation (how `build()`, `update()`, `draw()` work)
- `lib/include/bg2e/render/EnvironmentResources.hpp` — environment resources (IBL, skybox texture)
- `lib/include/bg2e/render/vulkan/Image.hpp` — `createAllocatedImage()`, `cmdCopy()`, `cmdTransitionImage()`
- `lib/include/bg2e/render/vulkan/macros/graphics.hpp` — `cmdClearImageAndBeginRendering()`, `cmdSetDefaultViewportAndScissor()`
- `lib/include/bg2e/render/ColorAttachments.cpp` — pattern for image creation with usage flags
