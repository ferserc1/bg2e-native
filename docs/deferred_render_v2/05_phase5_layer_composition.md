# Phase 5 — Layer Composition

## Objective

Combine all layers in `RendererDeferred` to produce the final rendered image. Implement the full rendering pipeline: background → opaque → transparent → output.

---

## Important: Build Policy

**DO NOT compile or build the project.** The user will personally review and test each implementation step.

---

## Sub-phases

### 5.1 — Layer Management in RendererDeferred

Add the following members to `RendererDeferred`:

```cpp
protected:
    // Layers
    std::unique_ptr<SkyboxLayer> _skyboxLayer;
    std::unique_ptr<DeferredLayer> _opaqueLayer;
    std::unique_ptr<DeferredLayer> _transparentLayer;

    // Intermediate images
    std::shared_ptr<vulkan::Image> _skyboxImage;    // Skybox output
    std::shared_ptr<vulkan::Image> _opaqueImage;    // Opaque layer output
    // Transparent layer writes directly to swapchain output

    // Environment and scene resources
    std::unique_ptr<EnvironmentResources> _environment;

    // Data bindings (shared across layers, configured on layers via setters)
    std::unique_ptr<vulkan::rt::RayTracingSceneDataBinding> _rtDataBinding;
    scene::vk::LightDataBinding::LightUniforms _lightUniforms;

    // Visitors
    scene::UpdateVisitor _updateVisitor;
    scene::ResizeViewportVisitor _resizeVisitor;

    // Selection highlight
    std::unique_ptr<manipulation::SelectionHighlight> _selectionHighlight;
```

### 5.2 — Full build() Implementation

```cpp
void RendererDeferred::build(Engine* engine, VkExtent2D initialExtent,
                             VkFormat colorImageFormat, VkFormat depthImageFormat,
                             VkSampleCountFlagBits sampleCount, bool isOffscreen) {
    _engine = engine;
    _viewportExtent = initialExtent;
    _colorImageFormat = colorImageFormat;
    _depthImageFormat = depthImageFormat;
    _isOffscreen = isOffscreen;

    // Create scene
    _scene = std::make_unique<scene::Scene>();

    // Create environment resources (IBL + skybox)
    _environment = std::make_unique<EnvironmentResources>(_engine);

    // Create RT data binding (shared across deferred layers)
    if (_engine->rayTracingSupported()) {
        _rtDataBinding = std::make_unique<vulkan::rt::RayTracingSceneDataBinding>(_engine);
    }

    // Create layers
    _skyboxLayer = std::make_unique<SkyboxLayer>(_engine);
    _skyboxLayer->build(initialExtent, colorImageFormat);
    _skyboxLayer->setScene(_scene.get());
    _skyboxLayer->setEnvironment(_environment.get());

    _opaqueLayer = std::make_unique<DeferredLayer>(_engine, LayerType::Opaque);
    _opaqueLayer->build(initialExtent, colorImageFormat);
    _opaqueLayer->setScene(_scene.get());
    _opaqueLayer->setEnvironment(_environment.get());
    _opaqueLayer->setRtDataBinding(_rtDataBinding.get());

    _transparentLayer = std::make_unique<DeferredLayer>(_engine, LayerType::Transparent);
    _transparentLayer->build(initialExtent, colorImageFormat);
    _transparentLayer->setScene(_scene.get());
    _transparentLayer->setEnvironment(_environment.get());
    _transparentLayer->setRtDataBinding(_rtDataBinding.get());

    // Create intermediate images
    _skyboxImage = vulkan::Image::createAllocatedImage(
        _engine, colorImageFormat, initialExtent,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    _opaqueImage = vulkan::Image::createAllocatedImage(
        _engine, colorImageFormat, initialExtent,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    // Selection highlight (non-offscreen only)
    if (!isOffscreen) {
        _selectionHighlight = std::make_unique<manipulation::SelectionHighlight>();
        // ... build selection highlight
    }
}
```

### 5.3 — Full draw() Implementation

```cpp
void RendererDeferred::draw(VkCommandBuffer cmd, uint32_t currentFrame,
                            const vulkan::Image* colorImage,
                            const vulkan::Image* depthImage,
                            const vulkan::Image* msaaDepthImage,
                            vulkan::FrameResources& frameResources) {
    _scene->willDraw();

    // Update ray tracing TLAS
    if (frameResources.rayTracingScene && _rtDataBinding) {
        frameResources.rayTracingScene->update(cmd, _scene->rootNode());
    }

    // Update environment (IBL, skybox texture if changed)
    _environment->update(cmd, currentFrame, frameResources);

    // Update light uniforms and configure deferred layers
    auto lightComponents = _scene->lightComponents();
    _lightUniforms.lightCount = 0;
    for (auto& lc : lightComponents) {
        if (_lightUniforms.lightCount >= BG2E_MAX_FORWARD_LIGHTS) break;
        auto& light = _lightUniforms.lights[_lightUniforms.lightCount];
        // ... fill light data from lc->light()
        _lightUniforms.lightCount++;
    }
    _opaqueLayer->setLightUniforms(_lightUniforms);
    _transparentLayer->setLightUniforms(_lightUniforms);

    // === Layer 1: Skybox ===
    _skyboxLayer->render(cmd, currentFrame, nullptr, _skyboxImage.get(),
                         frameResources);

    // === Layer 2: Opaque ===
    _opaqueLayer->render(cmd, currentFrame, _skyboxImage.get(), _opaqueImage.get(),
                         frameResources);

    // === Layer 3: Transparent (writes to swapchain output) ===
    _transparentLayer->render(cmd, currentFrame, _opaqueImage.get(), colorImage,
                              frameResources);

    // === Selection highlight (non-offscreen only) ===
    if (!_isOffscreen && _selectionHighlight) {
        auto mainCamera = _scene->mainCamera();
        auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
        auto projMatrix = mainCamera->projectionMatrix();
        _selectionHighlight->draw(_scene->rootNode(), viewMatrix, projMatrix, cmd);
    }

    _scene->didDraw();
}
```

### 5.4 — Full initScene() Implementation

```cpp
void RendererDeferred::initScene(std::shared_ptr<scene::Node> sceneRoot) {
    _scene->setSceneRoot(sceneRoot);

    // Build environment with default sky dome
    _environment->build(/* default path */);

    // Update scene lights
    _scene->updateLights();

    // Set up resize visitor
    _resizeVisitor.setViewportSize(_viewportExtent.width, _viewportExtent.height);
    _resizeVisitor.visit(_scene->rootNode());
}
```

### 5.5 — Full update() Implementation

```cpp
void RendererDeferred::update(float delta) {
    _scene->willUpdate();

    // Update scene visitors
    _updateVisitor.update(_scene->rootNode(), delta);

    // Update environment texture if changed
    auto mainEnv = _scene->mainEnvironment();
    if (mainEnv && mainEnv->environmentTexture()) {
        // Check if environment changed and swap if needed
        // ... same pattern as forward renderer
    }

    _scene->didUpdate();
}
```

### 5.6 — Full resize() Implementation

```cpp
void RendererDeferred::resize(VkExtent2D newExtent) {
    _viewportExtent = newExtent;
    _scene->willResize();

    // Resize layers
    _skyboxLayer->resize(newExtent);
    _opaqueLayer->resize(newExtent);
    _transparentLayer->resize(newExtent);

    // Recreate intermediate images
    _skyboxImage = vulkan::Image::createAllocatedImage(
        _engine, _colorImageFormat, newExtent, ...);
    _opaqueImage = vulkan::Image::createAllocatedImage(
        _engine, _colorImageFormat, newExtent, ...);

    // Update resize visitor
    _resizeVisitor.setViewportSize(newExtent.width, newExtent.height);
    _resizeVisitor.visit(_scene->rootNode());

    _scene->didResize();
}
```

### 5.7 — Full cleanup() Implementation

```cpp
void RendererDeferred::cleanup() {
    // Cleanup in reverse order of creation
    _selectionHighlight.reset();

    _transparentLayer->cleanup();
    _opaqueLayer->cleanup();
    _skyboxLayer->cleanup();

    _opaqueImage.reset();
    _skyboxImage.reset();

    _environment->cleanup();

    _rtDataBinding.reset();

    _engine->cleanupManager().push([&](VkDevice) {
        _scene.reset();
    });
}
```

### 5.8 — Code Review Checklist

- [ ] `draw()` executes all layers in correct order: skybox → opaque → transparent
- [ ] Intermediate images are created with correct format and usage flags
- [ ] Skybox renders to `_skyboxImage`
- [ ] Opaque layer receives `_skyboxImage` as input, outputs to `_opaqueImage`
- [ ] Transparent layer receives `_opaqueImage` as input, outputs to `colorImage` (swapchain)
- [ ] Light uniforms are computed from scene lights and configured on deferred layers via `setLightUniforms()`
- [ ] RT data binding is configured on deferred layers via `setRtDataBinding()` in `build()`
- [ ] All layers have `setScene()` and `setEnvironment()` called in `build()`
- [ ] Each layer extracts camera matrices from `_scene` inside its own `render()`
- [ ] `render()` calls use unified 5-parameter signature
- [ ] Selection highlight renders after all layers (non-offscreen only)
- [ ] `initScene()` builds environment and sets up visitors
- [ ] `update()` runs scene visitors and checks for environment changes
- [ ] `resize()` recreates intermediate images and updates visitors
- [ ] `cleanup()` releases resources in correct order
- [ ] No changes to existing forward renderer behavior

---

## Shader Development Notes

> **IMPORTANT:** No new shaders are needed in this phase. The layer composition uses the shaders created in Phase 4.

---

## Existing Code References

- `lib/src/bg2e/render/RendererBasicForward.cpp` — `draw()`, `initScene()`, `update()`, `resize()`, `cleanup()` patterns
- `lib/include/bg2e/render/RendererBasicForward.hpp` — member variables and structure
- `lib/include/bg2e/scene/Scene.hpp` — scene lifecycle methods (`willDraw`, `didDraw`, etc.)
- `lib/include/bg2e/render/EnvironmentResources.hpp` — environment build/update/draw
- `lib/include/bg2e/manipulation/SelectionHighlight.hpp` — selection highlight rendering
