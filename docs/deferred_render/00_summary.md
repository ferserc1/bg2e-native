# Plan de Implementación — Deferred Renderer

## Resumen General

Implementar un renderer deferred (tiled/forward+) dentro del engine bg2e-native, siguiendo una progresión incremental desde cero hasta un pipeline funcional con:

- **G-Buffer pass:** renderizado de geometría en múltiples attachments (albedo, normales tangenciales, propiedades materiales).
- **Compositing pass:** cálculo de iluminación por pixel leyendo los G-buffers, con soporte de sombras via ray tracing (cuando disponible) o sin sombras (fallback).
- **Transparent objects:** renderizados en una tercera pasada directa al swapchain tras la compositing.

---

## Fases del Plan

| Fase | Título | Descripción breve | Fichero de detalle |
|------|--------|-------------------|--------------------|
| 0 | `RendererDeferred` — shell vacío | Clase vacía con stubs de todos los métodos virtuales para verificar que el wiring del template delegate funciona. | [01_renderer_shell.md](./01_renderer_shell.md) |
| 1 | `GpuAttachmentBuffer` — G-Buffer infrastructure | Clase que gestiona la creación, formato, MSAA y gestión del ciclo de vida de las imágenes de los G-buffers. | [02_gpu_attachment_buffer.md](./02_gpu_attachment_buffer.md) |
| 2 | `DeferredCompositor` — compositing pass | Clase que gestiona el pipeline del pass de compositora, incluyendo shaders GLSL para leer los G-buffers y calcular iluminación por pixel (con o sin ray tracing). | [03_deferred_compositor.md](./03_deferred_compositor.md) |
| 3 | `RendererDeferred` — implementación completa | Implementación real de los métodos de `RendererDeferred`: draw con G-buffer pass → compositing pass → transparent passthrough. | [04_renderer_deferred_impl.md](./04_renderer_deferred_impl.md) |
| 4 | Integration — template expansion | Template instantiation explícita para `RendererDeferred` en `DefaultRenderLoopDelegate` y `DefaultOffscreenApplicationDelegate`. | [05_integration.md](./05_integration.md) |

---

## Notas técnicas clave

### Decisiones de diseño tomadas durante la planificación del análisis:

**Formatos de G-Buffer:**
| Attachment | Formato | Contenido |
|------------|---------|-----------|
| Color 0 | `VK_FORMAT_R8G8B8A8_UNORM` | Albedo (sin tono, SRGB en compositora) |
| Color 1 | `VK_FORMAT_R8G8B8A8_SNORM` | Normales en espacio tangencial (XYZ → [-1,1]) |
| Color 2 | `VK_FORMAT_R8G8B8A8_UNORM` | Metalness (R), Roughness (G), AO (B), Emissive (A) |

**MSAA:**
- Todos los attachments de los G-buffers (color + depth) usan 4x MSAA.
- Resolución al formato single-sample se realiza antes del compositing pass via `cmdResolveImage()`.

**Sombras:**
- Si el hardware soporta ray tracing → el compositor usa `rayQueryEXT` (mismo enfoque que `basic_forward_rt_shadows.frag.glsl`).
- Si no hay RT → todos los luces contribuyen sin test de sombra (igual que `basic_forward.frag.glsl`).

**Transparent objects:**
- Se renderizan en tercera pasada directa al swapchain tras el compositing, con alpha blending.
- Esto evita problemas de coherencia en G-buffers con objetos transparentes.

---

## Archivos necesarios (nuevos + modificados)

### Nuevos:
- `lib/include/bg2e/render/RendererDeferred.hpp` (fase 0)
- `lib/src/bg2e/render/RendererDeferred.cpp` (fase 0)
- `lib/include/bg2e/render/GpuAttachmentBuffer.hpp` (fase 1)
- `lib/src/bg2e/render/GpuAttachmentBuffer.cpp` (fase 1)
- `lib/include/bg2e/render/DeferredCompositor.hpp` (fase 2)
- `lib/src/bg2e/render/DeferredCompositor.cpp` (fase 2)
- `shaders/src/deferred_lighting.vert.glsl` (fase 2)
- `shaders/src/deferred_lighting.frag.glsl` (fase 2)

### Modificados:
- `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp` (fase 4)
- `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp` (fase 4)
- `lib/include/bg2e/render/all.hpp` (fase 4 — includes nuevos headers)
