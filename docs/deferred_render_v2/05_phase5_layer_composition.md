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

    // Data bindings (shared across deferred layers)
    std::unique_ptr<scene::vk::LightDataBinding> _lightDataBinding;
    std::unique_ptr<vulkan::rt::RayTracingSceneDataBinding> _rtDataBinding;

    // Selection highlight
    std::unique_ptr<manipulation::SelectionHighlight> _selectionHighlight;
```

> **NOTE (Refactoring):** The following members are now inherited from `Renderer` base class
> and should NOT be declared in `RendererDeferred`:
> - `_scene` — owned by `Renderer`
> - `_environment` — owned by `Renderer`
> - `_lightUniforms` — owned by `Renderer`, populated by `updateScene()`
> - `_resizeVisitor` — owned by `Renderer`
> - `_updateVisitor` — owned by `Renderer`
> - `_renderQueueVisitor` — owned by `Renderer`
> - `_renderQueue` — owned by `Renderer`
> - `_brightness`, `_contrast`, `_exposure` — owned by `Renderer`

### 5.2 — Full build() Implementation

```cpp
void RendererDeferred::build(Engine* engine, VkExtent2D initialExtent,
                             VkFormat colorImageFormat, VkFormat depthImageFormat,
                             VkSampleCountFlagBits sampleCount, bool isOffscreen) {
    // Force VK_SAMPLE_COUNT_1_BIT for deferred (no MSAA)
    sampleCount = VK_SAMPLE_COUNT_1_BIT;

    _engine = engine;
    _viewportExtent = initialExtent;
    _colorImageFormat = colorImageFormat;
    _depthImageFormat = depthImageFormat;
    _sampleCount = sampleCount;
    _isOffscreen = isOffscreen;

    // Create scene (owned by Renderer base)
    _scene = std::make_unique<scene::Scene>();

    // Create environment resources (owned by Renderer base)
    _environment = std::unique_ptr<EnvironmentResources>(
        new EnvironmentResources(_engine, { colorImageFormat }, depthImageFormat, sampleCount)
    );

    // Create shared data bindings for deferred layers
    _lightDataBinding = std::make_unique<scene::vk::LightDataBinding>(_engine);
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
    _opaqueLayer->setLightDataBinding(_lightDataBinding.get());
    _opaqueLayer->setRenderQueue(&_renderQueue);  // shared from Renderer base
    if (_rtDataBinding) _opaqueLayer->setRtDataBinding(_rtDataBinding.get());

    _transparentLayer = std::make_unique<DeferredLayer>(_engine, LayerType::Transparent);
    _transparentLayer->build(initialExtent, colorImageFormat);
    _transparentLayer->setScene(_scene.get());
    _transparentLayer->setEnvironment(_environment.get());
    _transparentLayer->setLightDataBinding(_lightDataBinding.get());
    _transparentLayer->setRenderQueue(&_renderQueue);  // shared from Renderer base
    if (_rtDataBinding) _transparentLayer->setRtDataBinding(_rtDataBinding.get());

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
        _selectionHighlight->init(engine);
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
    // === Scene preparation (from Renderer base) ===
    // This calls _scene->willDraw(), updates RT TLAS, updates environment,
    // updates skybox matrices, and populates the shared _renderQueue
    prepareSceneRender(cmd, currentFrame, frameResources);

    // Configure light uniforms on deferred layers
    // _lightUniforms is populated by updateScene() (called in update())
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

    // === End scene render (from Renderer base) ===
    endSceneRender();
}
```

> **NOTE (Refactoring):** `prepareSceneRender()` (from `Renderer` base) handles:
> - `_scene->willDraw()`
> - RT TLAS update
> - Environment update (IBL, skybox texture if changed)
> - Skybox color correction and matrix update
> - Render queue population from scene graph
>
> `endSceneRender()` handles:
> - `_scene->didDraw()`
>
> The `_lightUniforms` are populated by `updateScene()` which is called from `update()`.
> `RendererDeferred::draw()` only needs to pass them to the deferred layers.

### 5.4 — Full initScene() Implementation

```cpp
void RendererDeferred::initScene(std::shared_ptr<scene::Node> sceneRoot) {
    _scene->setSceneRoot(sceneRoot);

    // Build environment with default sky dome (same pattern as forward renderer)
    auto skyDomeTexture = std::make_shared<bg2e::base::Texture>();
    auto skyDomeGenerator = new bg2e::scene::SkyDomeTextureGenerator(2048, 1024, 4);
    skyDomeTexture->setProceduralGenerator(skyDomeGenerator);
    skyDomeTexture->setUseMipmaps(false);
    auto envTexture = std::make_shared<bg2e::render::Texture>(_engine);
    envTexture->load(skyDomeTexture);
    _environment->build(envTexture, { 2048, 2048 }, { 32, 32 }, { 1024, 1024 });

    // Update scene lights
    _scene->updateLights();

    // Set up resize visitor (from Renderer base)
    _resizeVisitor.resizeViewport(_scene->rootNode(), _viewportExtent);

    // Register cleanup for scene
    _engine->cleanupManager().push([&](VkDevice) {
        _scene.reset();
    });
}
```

> **NOTE (Refactoring):** `_resizeVisitor` is now in `Renderer` base class.
> The environment build pattern follows `RendererBasicForward::initScene()`.

### 5.5 — Full update() Implementation

```cpp
void RendererDeferred::update(float delta) {
    // Use the base class's updateScene() which handles:
    // - _scene->willUpdate()
    // - _updateVisitor.update() (from Renderer base)
    // - Environment texture swap if changed
    // - Light uniforms population (_lightUniforms)
    // - _scene->didUpdate()
    updateScene(delta, BG2E_MAX_FORWARD_LIGHTS);
}
```

> **NOTE (Refactoring):** All update logic is now in `Renderer::updateScene()`.
> `RendererDeferred::update()` is a simple delegation to the base class method.

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
        _engine, _colorImageFormat, newExtent,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    _opaqueImage = vulkan::Image::createAllocatedImage(
        _engine, _colorImageFormat, newExtent,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    // Update resize visitor (from Renderer base) and apply to scene
    _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);

    _scene->didResize();
}
```

> **NOTE (Refactoring):** `_resizeVisitor` is now in `Renderer` base class.
> The `_resizeVisitor.resizeViewport()` call follows the same pattern as `RendererBasicForward::resize()`.

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

    _rtDataBinding.reset();
    _lightDataBinding.reset();

    // Note: _environment, _scene, _renderQueue are cleaned up by Renderer base
    // or by the cleanup manager callback registered in initScene()
}
```

### 5.8 — Code Review Checklist

- [ ] `draw()` uses `prepareSceneRender()` and `endSceneRender()` from `Renderer` base
- [ ] `_lightUniforms` (from `Renderer` base) are passed to deferred layers via `setLightUniforms()`
- [ ] `_renderQueue` (from `Renderer` base) is shared with deferred layers via `setRenderQueue()` in `build()`
- [ ] `_lightDataBinding` is owned by `RendererDeferred` and shared with layers via `setLightDataBinding()`
- [ ] `_resizeVisitor` (from `Renderer` base) is used in `resize()` and `initScene()`
- [ ] `update()` delegates to `updateScene()` from `Renderer` base
- [ ] `initScene()` builds environment following `RendererBasicForward` pattern
- [ ] SkyboxLayer, DeferredLayer (opaque), DeferredLayer (transparent) render in correct order
- [ ] Intermediate images are created with correct format and usage flags
- [ ] Skybox renders to `_skyboxImage`
- [ ] Opaque layer receives `_skyboxImage` as input, outputs to `_opaqueImage`
- [ ] Transparent layer receives `_opaqueImage` as input, outputs to `colorImage` (swapchain)
- [ ] RT data binding is configured on deferred layers via `setRtDataBinding()` in `build()`
- [ ] All layers have `setScene()` and `setEnvironment()` called in `build()`
- [ ] Each layer extracts camera matrices from `_scene` inside its own `render()`
- [ ] `render()` calls use unified 5-parameter signature
- [ ] Selection highlight renders after all layers (non-offscreen only)
- [ ] `cleanup()` releases resources in correct order
- [ ] No changes to existing forward renderer behavior
- [ ] `RendererDeferred` does NOT duplicate members from `Renderer` base class

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
