# Phase 1 — RendererDeferred Shell + Example Project

## Objective

Create the `RendererDeferred` class as an empty skeleton that compiles and integrates with the engine's template delegate system. Simultaneously create a new example project to verify the wiring works. In this phase, `draw()` simply clears the screen to a solid color.

---

## Important: Build Policy

**DO NOT compile or build the project.** The user will personally review and test each implementation step.

---

## Sub-phases

### 1.1 — Create Example Project Structure

Create `examples/XX_deferred_renderer/` following the same pattern as existing examples (e.g., `examples/12_scene_renderer/`).

**File:** `examples/XX_deferred_renderer/src/main.cpp`

```cpp
#include <bg2e/render/all.hpp>
#include <bg2e/app/Application.hpp>

class DeferredSceneDelegate
    : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>
    , public bg2e::app::InputDelegate
{
protected:
    std::shared_ptr<bg2e::scene::Node> createScene() override {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        // Minimal scene: camera + environment
        return sceneRoot;
    }
};

class MyApplication : public bg2e::app::Application {
    void init(int argc, char** argv) override {
        auto delegate = std::make_shared<DeferredSceneDelegate>();
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
    }
};

int main(int argc, char** argv) {
    MyApplication app;
    app.run(argc, argv);
    return 0;
}
```

### 1.2 — Create RendererDeferred Header

**File:** `lib/include/bg2e/render/RendererDeferred.hpp`

Class inheriting from `bg2e::render::Renderer`, declaring all virtual methods with identical signatures to the base class.

**Structure:**
```
RendererDeferred.hpp
├── GPL license header (same as RendererBasicForward.hpp)
├── #pragma once
├── Includes: Renderer.hpp, Engine.hpp, scene/Scene.hpp, vulkan/Image.hpp
├── Public methods (all overrides from Renderer):
│   ├── build(), initFrameResources(), initScene(), resize()
│   ├── update(), draw(), cleanup()
│   ├── scene()
│   ├── setBrightness/brightness, setContrast/contrast, setExposure/exposure
│   └── viewportWidth(), viewportHeight()
├── Protected members:
│   ├── Engine* _engine
│   ├── VkExtent2D _viewportExtent
│   ├── VkFormat _colorImageFormat
│   ├── VkFormat _depthImageFormat
│   ├── bool _isOffscreen
│   ├── std::unique_ptr<scene::Scene> _scene
│   ├── float _brightness, _contrast, _exposure
│   └── (empty — to be filled in later phases)
```

**Include dependencies:** See `lib/include/bg2e/render/RendererBasicForward.hpp` for the exact include pattern.

**BG2E_API macro:** All public symbols must use the `BG2E_API` macro from `bg2e/common.hpp`.

### 1.3 — Create RendererDeferred Implementation

**File:** `lib/src/bg2e/render/RendererDeferred.cpp`

All methods are stubs. `draw()` clears the color image to a solid color (e.g., dark blue `{0.1, 0.1, 0.2, 1.0}`) using the existing macro `vulkan::macros::cmdClearImageAndBeginRendering()`.

```cpp
void RendererDeferred::draw(VkCommandBuffer cmd, uint32_t currentFrame,
                            const vulkan::Image* colorImage,
                            const vulkan::Image* depthImage,
                            const vulkan::Image* msaaDepthImage,
                            vulkan::FrameResources& frameResources) {
    VkClearColorValue clearValue{ { 0.1f, 0.1f, 0.2f, 1.0f } };
    vulkan::macros::cmdClearImageAndBeginRendering(cmd, colorImage, clearValue);
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, colorImage->extent2D());
    vulkan::cmdEndRendering(cmd);
}
```

**Stub implementations for all other methods:**
- `build()`: Store engine pointer and parameters. Create `Scene` instance.
- `initFrameResources()`: Empty.
- `initScene()`: Call `_scene->setSceneRoot(sceneRoot)`.
- `resize()`: Update `_viewportExtent`.
- `update()`: Call `_scene->willUpdate()` / `_scene->didUpdate()`.
- `cleanup()`: Reset `_scene`.
- `scene()`: Return `_scene.get()`.
- Color correction: Store/return `_brightness`, `_contrast`, `_exposure`.
- Viewport: Return `_viewportExtent.width` / `height`.

### 1.4 — Code Review Checklist

- [ ] `RendererDeferred.hpp` follows the same structure as `RendererBasicForward.hpp`
- [ ] All virtual methods from `Renderer` base class are declared with correct signatures
- [ ] `BG2E_API` macro is applied to the class declaration
- [ ] License header matches the project convention
- [ ] `RendererDeferred.cpp` has stub implementations for all methods
- [ ] `draw()` uses `vulkan::macros::cmdClearImageAndBeginRendering()` correctly
- [ ] Example project follows the same pattern as `examples/12_scene_renderer/`
- [ ] No changes to existing forward renderer behavior

---

## Existing Code References

- `lib/include/bg2e/render/Renderer.hpp` — base class interface (pure virtual methods)
- `lib/include/bg2e/render/RendererBasicForward.hpp` — header structure to follow
- `lib/src/bg2e/render/RendererBasicForward.cpp` — implementation patterns to follow
- `lib/include/bg2e/render/vulkan/macros/graphics.hpp` — `cmdClearImageAndBeginRendering()` macro
- `examples/12_scene_renderer/src/main.cpp` — example pattern to follow

## Notes

- The template delegate `DefaultRenderLoopDelegate<RendererDeferred>` requires explicit template instantiation (Phase 6). Until then, the example can use the raw `RenderLoopDelegate` pattern (like `examples/11_scene_renderer_raw/`), or Phase 6 can be done partially here.
- The `draw()` signature includes `msaaDepthImage` which is unused in the deferred renderer (no MSAA). It is kept for interface compatibility with the base class.
