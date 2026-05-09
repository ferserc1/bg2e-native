# Phase 6 — Integration

## Objective

Extend `DefaultRenderLoopDelegate<RendererT>` and `DefaultOffscreenApplicationDelegate<RendererT>` to work with `RendererDeferred` via explicit template instantiation.

---

## Important: Build Policy

**DO NOT compile or build the project.** The user will personally review and test each implementation step.

---

## Sub-phases

### 6.1 — Explicit Template Instantiation for DefaultRenderLoopDelegate

**File:** `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp`

Add at the end of the file, after the existing `RendererBasicForward` instantiations:

```cpp
// Explicit template instantiation for RendererDeferred
template class BG2E_API DefaultRenderLoopDelegate<RendererDeferred>;

template DefaultRenderLoopDelegate<RendererDeferred>::~DefaultRenderLoopDelegate();
template void DefaultRenderLoopDelegate<RendererDeferred>::init(render::Engine*);
template void DefaultRenderLoopDelegate<RendererDeferred>::initFrameResources(render::vulkan::DescriptorSetAllocator*);
template void DefaultRenderLoopDelegate<RendererDeferred>::initScene();
template void DefaultRenderLoopDelegate<RendererDeferred>::swapchainResized(VkExtent2D);
template void DefaultRenderLoopDelegate<RendererDeferred>::update(uint32_t, render::vulkan::FrameResources&);
template VkImageLayout DefaultRenderLoopDelegate<RendererDeferred>::render(
    VkCommandBuffer, uint32_t,
    const render::vulkan::Image*, const render::vulkan::Image*,
    const render::vulkan::Image*, render::vulkan::FrameResources&);
template void DefaultRenderLoopDelegate<RendererDeferred>::cleanup();
template RendererDeferred* DefaultRenderLoopDelegate<RendererDeferred>::renderer();
```

**Also add the include at the top of the file:**
```cpp
#include <bg2e/render/RendererDeferred.hpp>
```

### 6.2 — Explicit Template Instantiation for DefaultOffscreenApplicationDelegate

**File:** `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp`

Add at the end of the file, after the existing `RendererBasicForward` instantiations:

```cpp
// Explicit template instantiation for RendererDeferred
template class BG2E_API DefaultOffscreenApplicationDelegate<RendererDeferred>;

template void DefaultOffscreenApplicationDelegate<RendererDeferred>::initConfig(int, char**, app::OffscreenApplicationConfig&);
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::init(Engine*, std::shared_ptr<vulkan::Image>, std::shared_ptr<vulkan::Image>);
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::initFrameResources(vulkan::DescriptorSetAllocator*);
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::initScene();
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::update(uint32_t, vulkan::FrameResources&);
template VkImageLayout DefaultOffscreenApplicationDelegate<RendererDeferred>::render(
    VkCommandBuffer, uint32_t,
    const vulkan::Image*, const vulkan::Image*,
    const vulkan::Image*, vulkan::FrameResources&);
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::cleanup();
template RendererDeferred* DefaultOffscreenApplicationDelegate<RendererDeferred>::renderer();
```

**Also add the include at the top of the file:**
```cpp
#include <bg2e/render/RendererDeferred.hpp>
```

### 6.3 — Update all.hpp

**File:** `lib/include/bg2e/render/all.hpp`

Add includes for new headers at the end of the file:

```cpp
// Deferred renderer
#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/deferred/RenderLayer.hpp>
#include <bg2e/render/deferred/DeferredLayer.hpp>
#include <bg2e/render/deferred/SkyboxLayer.hpp>
```

### 6.4 — Update Example to Use Template Delegate

Update `examples/XX_deferred_renderer/src/main.cpp` to use `DefaultRenderLoopDelegate<RendererDeferred>`:

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

        // Add camera
        auto cameraNode = std::make_shared<bg2e::scene::Node>("Camera");
        auto camera = new bg2e::scene::CameraComponent();
        cameraNode->addComponent(camera);
        sceneRoot->addChild(cameraNode);

        // Add environment
        auto envNode = std::make_shared<bg2e::scene::Node>("Environment");
        auto env = new bg2e::scene::EnvironmentComponent();
        envNode->addComponent(env);
        sceneRoot->addChild(envNode);

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

### 6.5 — Code Review Checklist

- [ ] Explicit template instantiation compiles for `DefaultRenderLoopDelegate<RendererDeferred>`
- [ ] Explicit template instantiation compiles for `DefaultOffscreenApplicationDelegate<RendererDeferred>`
- [ ] `#include <bg2e/render/RendererDeferred.hpp>` is added to both `.cpp` files
- [ ] `all.hpp` includes all new headers
- [ ] Example uses `DefaultRenderLoopDelegate<RendererDeferred>` correctly
- [ ] No changes to existing `RendererBasicForward` instantiations
- [ ] No linker errors (all virtual methods are implemented)
- [ ] No changes to existing forward renderer behavior

---

## Existing Code References

- `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp` — existing template instantiation pattern
- `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp` — existing template instantiation pattern
- `lib/include/bg2e/render/all.hpp` — existing include pattern
- `examples/12_scene_renderer/src/main.cpp` — example pattern to follow
- `examples/16_offscreen_scene_render_cli/src/main.cpp` — offscreen example pattern
