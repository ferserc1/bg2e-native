# Fase 0 — RendererDeferred (Shell Vacío)

## Objetivo

Crear la clase `bg2e::render::RendererDeferred` como skeleton que compila e integra correctamente con el sistema de templates del engine, sin modificar comportamiento alguno. Esta fase es prerequisito para verificar que el wiring de los templates (DefaultRenderLoopDelegate, DefaultOffscreenApplicationDelegate) funciona antes de implementar la lógica real.

---

## Ficheros a crear

### 1. `lib/include/bg2e/render/RendererDeferred.hpp`

Clase que hereda de `bg2e::render::Renderer`, declarando todos los métodos virtuales con signatura idéntica a la clase base. El cuerpo de cada método se dejará para implementación posterior.

```
lib/include/bg2e/render/RendererDeferred.hpp
├── Inherits: bg2e::render::Renderer (abstract)
├── GlslIncludes: Renderer.hpp, Engine.hpp, Scene/Node.hpp, Vulkan/Image.hpp
├── Public methods: build(), initFrameResources(), initScene(), resize(),
│                   update(), draw(), cleanup(), scene(), setters/generators de tonemapping,
│                   viewportWidth()/viewportHeight()
└── Protected members: (vacío — se llenarán en fases posteriores)
```

**Dependencias entre includes:** Ver `lib/include/bg2e/render/Renderer.h*pp` para saber qué imports necesita.

### 2. `lib/src/bg2e/render/RendererDeferred.cpp`

Cuerpos vacíos de cada método, siguiendo la convención del proyecto (estructura con license header, namespace `bg2e::render`).

---

## Implementación detallada — métodos stub

### build()
```cpp
void BuilderDeferred::build(Engine* engine, VkExtent2D initialExtent,
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
Respecta la contract of the base class; redimensionará G-buffers en fases posteriores. En esta fase:
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

### draw() — el más importante
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

### Methods de tonemapping
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

## Checklist de integración

- [ ] La clase compila sin errores (`cmake --build build` funciona para RendererDeferred.cpp)
- [ ] La clase es instanciable vía `RendererT` template del DefaultRenderLoopDelegate
- [ ] No hay cambios en comportamiento existente (clase vacía no ejecuta nada)
- [ ] Los stubs permiten que el template delegate forward calls sin errores de linker

## Notas sobre patrones existentes a seguir

- La estructura del fichero `.hpp` debe imitar `lib/include/bg2e/render/RendererBasicForward.h*pp`:
  - Licenza GPL al inicio.
  - `#pragma once`
  - Includes de las clases usadas, namespace `bg2e::render::*`.
  - Macros `BG2E_API` en todos los symbols públicos (macro from `bg2e/common.hpp`).
  - Public section: constructors, destruktory, property getters/setters, lifecycle methods.
  - Protected section: members (_engine, _scene, pipeline handles, bindings...).

## Referencias de código existente para copiar la estructura

- `lib/include/bg2e/render/RendererBasicForward.hpp` — patrones de includes y estructura
- `lib/src/bg2e/render/RendererBasicForward.cpp` — patrones de implementación (copiar signatures exactas)
