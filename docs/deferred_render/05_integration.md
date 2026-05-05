# Fase 4 — Integration (Template Expansion)

## Objetivo

Extender el template `DefaultRenderLoopDelegate<RendererT>` para permitirlo usar con `RendererDeferred`, y de igual manera extender `DefaultOffscreenApplicationDelegate<RendererT>`.

Esto se hace mediante **explicit template instantiation** en los ficheros `.cpp` existentes, de forma análoga a como `RendererBasicForward` está integrado actualmente.

---

## Ficheros modificados

### A) `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp`

Añadir explicit template instantiation para `RendererDeferred` al final del fichero, justo después de las existentes:

```cpp
// lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp (existing code at bottom):

template class BG2E_API DefaultRenderLoopDelegate<RendererBasicForward>;
// [rest of the template functions already instantiated for RendererBasicForward...]

// ➡️ Add new explicit instantiation for:
template class BG2E_API DefaultRenderLoopDelegate<RendererDeferred>;

template DefaultRenderLoopDelegate<RendererDeferred>::~DefaultRenderLoopDelegate();
template void DefaultRenderLoopDelegate<RendererDeferred>::init(render::Engine * engine);
template void DefaultRenderLoopDelegate<RendererDeferred>::initFrameResources(render::vulkan::DescriptorSetAllocator*);
template void DefaultRenderLoopDelegate<RendererDeferred>::initScene();
template template void DefaultRenderLoopDelegate<RendererDeferred>::swapchainResized(VkExtent2D);
template void DefaultRenderLoopDelegate<RendererDeferred>::update(uint32_t currentFrame, render::vulkan::FrameResources&);
template VkImageLayout DefaultRenderLoopDelegate<RendererDeferred>::render(VkCommandBuffer cmd, uint32_t currentFrame, const render::vulkan::Image* colorImage, const render::vulkan::Image* depthImage, const render::vulkan.Image* msaaDepthImage, render::vulkan.FrameResources& frameResources);
template void DefaultRenderLoopDelegate<RendererDeferred>::cleanup();
template RendererDeferred* DefaultRenderLoopDelegate<RendererDeferred>::renderer();
```

### B) `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp`

Añadir explicit template instantiation para `RendererDeferred`:

```cpp
// lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp (bottom):

template class BG2E_API DefaultOffscreenApplicationDelegate<RendererBasicForward>;
// [rest of the template functions for RendererBasicForward...]

// ➡️ Add new explicit instantiation for:
template class BG2E_API DefaultOffscreenApplicationDelegate<RendererDeferred>;

// Plus all the explicit instantiation for each method (same pattern as above):
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::initConfig(int, char**, OffscreenApplicationConfig&);
template void DefaultOffscreenApplicationDelegate<RendererDeferred>::init(Engine*, std::shared_ptr<vulkan::Image> color, std::shared_ptr<vulkan::Image> depth);
... (all methods with explicit templates)
```

### C) `lib/include/bg2e/render/all.hpp`  (header convenience include)

Añadir includes de los nuevos headers al final:

```cpp
// lib/include/bg2e/render/all.hpp (existing includes):

#include <bg2e/render/RendererBasicForward.hpp>
// ... all existing headers...

// ➡️ Add new includes for deferred renderer:
#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/GpuAttachmentBuffer.hpp>
#include <bg2e/render/DeferredCompositor.hpp>
```

---

## Verificación de compatibilidad del template

La plantilla `DefaultRenderLoopDelegate` llama los métodos siguientes en RendererT:
- `init()`, `build()`, `initFrameResources()`, `initScene()`, `resize()`, `update()`, `draw()`, `cleanup()`
- Propiedades: `renderer()->*()` (access to the underlying renderer instance).

`RendererDeferred` implementa todos estos metodos (declarados en la fase 0). Por lo tanto, no se necesitan cambios en la plantilla base. Solo añadir explicit template instantiation.

**Mismos argumentos para `DefaultOffscreenApplicationDelegate`**: los métodos que llama (igual que arriba).

---

## Verificación final del build

Al compilar:
- `cmake --build build` debe generar los nuevos objects: `renderer_deferred.o`, `gpu_attachment_buffer.o`, `deferred_compositor.o`
- Los nuevos shaders: `deferred_lighting.vert.spv`, `deferred_lighting.frag.spv` (y `_rt_shadows variant if needed`)
- La aplicación cliente debe poder crear una instancia de `RendererDeferred` via:
  ```cpp
  auto delegate = std::make_shared<MyDelegate<RendererDeferred>>(...);
  // o: DefaultRenderLoopDelegate<RendererDeferred> (if we create MyDelegate subclass).
  ```

### Nuevo pipeline de build (CMake)
Ningún cambio en CMake. La build system de bg2e-native incluye automáticamente todos los `.hpp`, `.cpp`, `.glsl` en las carpetas `lib/src/`, `lib/include/`, `shaders/` (según instructions: "the project is con configured to auto-include all code files placed in...").

**Checklist de la fase 4:**
- [ ] `DefaultRenderLoopDelegate<RendererDeferred>` compila y enlaza correctamente.
- [ ] `DefaultOffscreenApplicationDelegate<RendererDeferred>` compila y enlaza correctamente.
- [ ] `#include <bg2e/render/all.hpp>` incluye los nuevos headers (`RendererDeferred`, `GpuAttachmentBuffer`, `DeferredCompositor`).
- [ ] No hay errores de linker (sintomas de símbolos no definidos).
- [ ] Aplicación que usa `RendererDeferred` compila: ejemplo mínimo `example_deferred.cpp`.

---

## Ejemplo mínimo de aplicación con RendererDeferred (opcional)

Para verificar que la integración funciona, se debe crear una simple aplicación de prueba:

```cpp
// examples/XX_deferred_test.cpp (ejemplo mínimo)
#include <bg2e/render/all.hpp>

class MyDeferredDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred> {
    virtual bg2e::scene::createScene() override { /* minimal test scene */ }
};

int main(int argc, char** argv) {
    // ... same pattern as examples using DefaultRenderLoopDelegate
}
```

> **Nota:** No se requiere crear un ejemplo de aplicación en esta fase. La verificación automática se hace con el build del proyecto.
