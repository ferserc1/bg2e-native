# Phase 0 — RendererDeferred (Empty Shell)

## Objective

Create the class `bg2e::render::RendererDeferred` as a skeleton that compiles and integrates correctly with the engine's template system, without modifying any behavior. This phase is a prerequisite to verify that the template wiring (DefaultRenderLoopDelegate, DefaultOffscreenApplicationDelegate) works before implementing real logic.

---

## Files to Create

### 1. `lib/include/bg2e/render/RendererDeferred.hpp`

Class inheriting from `bg2e::render::Renderer`, declaring all virtual methods with identical signatures to the base class. The body of each method will be left for later implementation.

```
lib/include/bg2e/render/RendererDeferred.hpp
├── Inherits: bg2e::render::Renderer (abstract)
├── GlslIncludes: Renderer.hpp, Engine.hpp, Scene/Node.hpp, Vulkan/Image.hpp
├── Public methods: build(), initFrameResources(), initScene(), resize(),
│                   update(), draw(), cleanup(), scene(), tonemapping setters/getters,
│                   viewportWidth()/viewportHeight()
└── Protected members: (empty — to be filled in later phases)
```

**Include dependencies:** See `lib/include/bg2e/render/Renderer.h*pp` for required imports.

### 2. `lib/src/bg2e/render/RendererDeferred.cpp`

Empty method bodies, following the project convention (structure with license header, namespace `bg2e::render`).

---

## Detailed Implementation — Stub Methods

### build()
```cpp
void RendererDeferred::build(Engine* engine, VkExtent2D initialExtent,
                            VkFormat colorImageFormat, VkFormat depthImageFormat,
                            VkSampleCountFlagBits sampleCount, bool isOffscreen) override {
    // TODO: implement G-buffer and compositor setup
}
```

### initFrameResources()
```cpp
void RendererDeferred::initFrameResources(DescriptorSetAllocator* frameAllocator) override {
    // TODO: init G-buffer and compositor frame resources
}
```

### initScene()
```cpp
void RendererDeferred::initScene(std::shared_ptr<scene::Node> sceneRoot) override {
    // TODO: init scene wrapper (copy pattern from RendererBasicForward.initScene())
}
```

### resize()
Respects the base class contract; will resize G-buffers in later phases. In this phase:
```cpp
void RendererDeferred::resize(VkExtent2D newExtent) override {
    // TODO: resize G-buffers and compositor buffers
}
```

### update()
```cpp
void RendererDeferred::update(float delta) override {
    // TODO: update scene graph and lights (copy pattern from RendererBasicForward.update())
}
```

### draw() — the most important
```cpp
void RendererDeferred::draw(VkCommandBuffer cmd, uint32_t currentFrame,
                            const vulkan::Image* colorImage,
                            const vulkan::Image* depthImage,
                            const vulkan::Image* msaaDepthImage,
                            FrameResources& frameResources) override {
    // TODO: implement G-buffer pass + compositing pass
}
```

### cleanup()
```cpp
void RendererDeferred::cleanup() override {
    // TODO: destroy G-buffer images, compositor pipeline
}
```

### Tonemapping methods
```cpp
void RendererDeferred::setBrightness(float b) override { /* TODO */ }
float RendererDeferred::brightness() const override { return 0.0f; } /* TODO: store state */
void RendererDeferred::setContrast(float c) override { /* TODO */ }
float RendererDeferred::contrast() const override { return 1.0f; } /* TODO: store state */
void RendererDeferred::setExposure(float e) override { /* TODO */ }
float RendererDeferred::exposure() const override { return 1.0f; } /* TODO: store state */
```

### Viewport properties
```cpp
uint32_t RendererDeferred::viewportWidth() override { /* TODO: return engine swapchain extent */ }
uint32_t RendererDeferred::viewportHeight() override { /* TODO: return engine swapchain extent */ }
```

### scene()
```cpp
scene::Scene* RendererDeferred::scene() override { return nullptr; } /* TODO: return _scene */
```

---

## Integration Checklist

- [ ] The class compiles without errors (`cmake --build build` works for RendererDeferred.cpp)
- [ ] The class is instantiable via the `RendererT` template of DefaultRenderLoopDelegate
- [ ] No changes to existing behavior (empty class executes nothing)
- [ ] The stubs allow the template delegate to forward calls without linker errors

## Notes on Existing Patterns to Follow

- The `.hpp` file structure must imitate `lib/include/bg2e/render/RendererBasicForward.h*pp`:
  - GPL license at the top.
  - `#pragma once`
  - Includes of used classes, namespace `bg2e::render::*`.
  - `BG2E_API` macros on all public symbols (macro from `bg2e/common.hpp`).
  - Public section: constructors, destructors, property getters/setters, lifecycle methods.
  - Protected section: members (_engine, _scene, pipeline handles, bindings...).

## Existing Code References for Structure

- `lib/include/bg2e/render/RendererBasicForward.hpp` — include patterns and structure
- `lib/src/bg2e/render/RendererBasicForward.cpp` — implementation patterns (copy exact signatures)
