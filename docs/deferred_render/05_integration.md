# Phase 4 — Integration (Template Expansion)

## Objective

Extend the `DefaultRenderLoopDelegate<RendererT>` template to allow it to work with `RendererDeferred`, and likewise extend `DefaultOffscreenApplicationDelegate<RendererT>`.

This is done via **explicit template instantiation** in the existing `.cpp` files, analogous to how `RendererBasicForward` is currently integrated.

---

## Modified Files

### A) `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp`

Add explicit template instantiation for `RendererDeferred` at the end of the file, right after the existing ones:

```cpp
// lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp (existing code at bottom):

template class BG2E_API DefaultRenderLoopDelegate<RendererBasicForward>;
// [rest of the template functions already instantiated for RendererBasicForward...]

// Add new explicit instantiation for:
template class BG2E_API DefaultRenderLoopDelegate<RendererDeferred>;

template DefaultRenderLoopDelegate<RendererDeferred>::~DefaultRenderLoopDelegate();
template void DefaultRenderLoopDelegate<RendererDeferred>::init(render::Engine * engine);
template void DefaultRenderLoopDelegate<RendererDeferred>::initFrameResources(render::vulkan::DescriptorSetAllocator*);
template void DefaultRenderLoopDelegate<RendererDeferred>::initScene();
template void DefaultRenderLoopDelegate<RendererDeferred>::swapchainResized(VkExtent2D);
template void DefaultRenderLoopDelegate<RendererDeferred>::update(uint32_t currentFrame, render::vulkan::FrameResources&);
template VkImageLayout DefaultRenderLoopDelegate<RendererDeferred>::render(VkCommandBuffer cmd, uint32_t currentFrame, const render::vulkan::Image* colorImage, const render::vulkan::Image* depthImage, const render::vulkan::Image* msaaDepthImage, render::vulkan::FrameResources& frameResources);
template void DefaultRenderLoopDelegate<RendererDeferred>::cleanup();
template RendererDeferred* DefaultRenderLoopDelegate<RendererDeferred>::renderer();
```

### B) `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp`

Add explicit template instantiation for `RendererDeferred`:

```cpp
// lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp (bottom):

template class BG2E_API DefaultOffscreenApplicationDelegate<RendererBasicForward>;
// [rest of the template functions for RendererBasicForward...]

// Add new explicit instantiation for:
template class BG2E_API DefaultOffscreenApplicationDelegate<RendererDeferred>;

// Plus all the explicit instantiation for each method (same pattern as above):
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::initConfig(int, char**, OffscreenApplicationConfig&);
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::init(Engine*, std::shared_ptr<vulkan::Image> color, std::shared_ptr<vulkan::Image> depth);
// ... (all methods with explicit templates)
```

### C) `lib/include/bg2e/render/all.hpp` (convenience include header)

Add includes for the new headers at the end:

```cpp
// lib/include/bg2e/render/all.hpp (existing includes):

#include <bg2e/render/RendererBasicForward.hpp>
// ... all existing headers...

// Add new includes for deferred renderer:
#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/GpuAttachmentBuffer.hpp>
#include <bg2e/render/DeferredCompositor.hpp>
```

---

## Template Compatibility Verification

The `DefaultRenderLoopDelegate` template calls the following methods on RendererT:
- `init()`, `build()`, `initFrameResources()`, `initScene()`, `resize()`, `update()`, `draw()`, `cleanup()`
- Properties: `renderer()->*()` (access to the underlying renderer instance).

`RendererDeferred` implements all these methods (declared in phase 0). Therefore, no changes to the base template are needed. Only explicit template instantiation needs to be added.

**Same arguments for `DefaultOffscreenApplicationDelegate`**: it calls the same methods as above.

---

## Final Build Verification

When compiling:
- `cmake --build build` should generate the new objects: `renderer_deferred.o`, `gpu_attachment_buffer.o`, `deferred_compositor.o`
- New shaders: `deferred_lighting.vert.spv`, `deferred_lighting.frag.spv` (and `_rt_shadows` variant if needed)
- The client application should be able to create a `RendererDeferred` instance via:
  ```cpp
  auto delegate = std::make_shared<MyDelegate<RendererDeferred>>(...);
  // or: DefaultRenderLoopDelegate<RendererDeferred> (if we create MyDelegate subclass).
  ```

### New Build Pipeline (CMake)
No changes to CMake. The bg2e-native build system automatically includes all `.hpp`, `.cpp`, `.glsl` files in the `lib/src/`, `lib/include/`, `shaders/` folders (per instructions: "the project is configured to auto-include all code files placed in...").

**Phase 4 Checklist:**
- [ ] `DefaultRenderLoopDelegate<RendererDeferred>` compiles and links correctly.
- [ ] `DefaultOffscreenApplicationDelegate<RendererDeferred>` compiles and links correctly.
- [ ] `#include <bg2e/render/all.hpp>` includes the new headers (`RendererDeferred`, `GpuAttachmentBuffer`, `DeferredCompositor`).
- [ ] No linker errors (symptoms of undefined symbols).
- [ ] Application using `RendererDeferred` compiles: minimal example `example_deferred.cpp`.

---

## Minimal Application Example with RendererDeferred (optional)

To verify the integration works, a simple test application should be created:

```cpp
// examples/XX_deferred_test.cpp (minimal example)
#include <bg2e/render/all.hpp>

class MyDeferredDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred> {
    virtual bg2e::scene::createScene() override { /* minimal test scene */ }
};

int main(int argc, char** argv) {
    // ... same pattern as examples using DefaultRenderLoopDelegate
}
```

> **Note:** Creating an application example is not required in this phase. Automatic verification is done via the project build.
