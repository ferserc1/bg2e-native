# Fase 1 — GpuAttachmentBuffer (G-Buffer Infrastructure)

## Objetivo

Crear una clase centralizada que gestione:
- Creación de imágenes Vulkan para los G-buffers (formatos, uso flags, MSAA).
- El resize de las imágenes cuando el swapchain cambia de tamaño.
- La gestión del ciclo de vida (destrucción limpia).
- Helpers para transición de layout y resolución MSAA.

Clase inspirada en `ColorAttachments` pero con soporte completo de MSAA y depth attachment.

---

## Fichero a crear: `lib/include/bg2e/render/GpuAttachmentBuffer.hpp`

```
GpuAttachmentBuffer.hpp
├── license header (GPL)
├── #pragma once
├── Depends: Engine.hpp, vulkan/Image.hpp (existing includes)
├── Public
│   ├── GpuAttachmentBuffer(Engine* engine, VkExtent2D extent) constructor
│   ├── ~GpuAttachmentBuffer() destructor → cleanup()
│   ├── build(VkExtent2D extent) → creates all G-buffer images
│   ├── cleanup() → destroys Vulkan resources of G-buffers
│   └── imageWithIndex(uint32_t index) → shared_ptr<vulkan::Image>
│   └── [property getters] formats(), extent(), size(), msaaCount()
├── Protected / Private (methods for creating images)
│   ├── _createColorAttachments() → G-buffer color attachments (3) with MSAA 4x
│   ├── _createDepthAttachment() → depth buffer with MSAA (same sample count as colors)
│   └── _createResolveImages() → single-sample resolve targets (one per color attachment)
├── Protected members
│   ├── Engine* _engine
│   ├── VkExtent2D _extent
│   ├── std::vector<std::shared_ptr<vulkan::Image>> _gbufferColors → G-buffer MSAA color images
│   ├── std::vector<std::shared_ptr<vulkan::Image>> _resolveTargets → single-sample resolve targets
│   ├── std::shared_ptr<vulkan::Image> _depthBuffer → MSAA depth
│   ├── VkSampleCountFlagBits _msaaSampleCount
│   └── std::vector<VkFormat> _formats (3+1: 3 colors + depth)
```

### Decisiones de formato detalladas

| Attachment | Index | Formato | Bits per pixel | Contenido final |
|------------|-------|---------|----------------|-----------------|
| Color 0 (albedo) | 0 | `VK_FORMAT_R8G8B8A8_UNORM` | 32 | Color base (sin SRGB encoding) |
| Color 1 (normales) | 1 | `VK_FORMAT_R8G8B8A8_SNORM` | 32 | Normales tangenciales (normal.x = (tex.r * 2) - 1) |
| Color 2 (materiales) | 2 | `VK_FORMAT_R8G8B8A8_UNORM` | 32 | Metalness(R), Roughness(G), AO(B), Emissive(A) |
| Depth prepass | — | `VK_FORMAT_D32_SFLOAT` | 32 | Profundidad single-sample para oclusión en compositing (opcional, see note below sobre depth) |

> **Nota: single-sample vs MSAA en depth.**
> El G-buffer depth attachment debe tener el count de muestras igual que los color attachments (dynamic rendering requiere sample counts iguales entre todos los attachments). Si usamos MSAA 4x en color, también debe ser 4x en depth.
> 
> **Para evitar esto**, podemos usar un enfoque híbrido: G-buffer depths en single-sample (depth prepass), y MSAA solo en los color attachments. Esto requiere dos draw calls por objeto: primero single-sample depth prepass, luego color-pass con depth testing (but no writing) contra ese buffer.
> 
> **Decisión adoptada:** Usar single-sample depth en el G-buffer pass (depth prepass), y MSAA solo en los color attachments. Esto es más eficiente porque:
> - El depth prepass single-sample es rápido (solo escribe profundidad).
> - Los G-buffers con MSAA mejoran la calidad en bordes de geometría.
> - No requiere MSAA resolve del depth buffer (single sample).
> 
> Para implementar esto, el draw debe tener dos render-passes: uno para depth-only (sin color attachments), otro para los G-buffer colors con depth-test-read (no-write) contra el depth prepass.

---

## Fichero a crear: `lib/src/bg2e/render/GpuAttachmentBuffer.cpp`

### Constructor
```cpp
GpuAttachmentBuffer::GpuAttachmentBuffer(Engine* engine, VkExtent2D extent)
    : _engine(engine), _extent(extent), _msaaSampleCount(VK_SAMPLE_COUNT_4_BIT) 
{
    _formats = { VK_FORMAT_R8G8B8A8_UNORM, /* ... */ };
}
```

### build() — create all G-buffer images
- Call `cleanup()` (idempotent, clear already-deallocated resources).
- For each color attachment:
  - Call `vulkan::Image::createAllocatedImage()` with format, extent, usage flags (`COLOR_ATTACHMENT | SAMPLED | TRANSFER_DST | TRANSFER_SRC`), aspect `VK_IMAGE_ASPECT_COLOR_BIT`, sampleCount = `_msaaSampleCount`.
  - Store in `_gbufferColors`.
- Create depth attachment: single sample, `VK_FORMAT_D32_SFLOAT`, usage = `DEPTH_STENCIL_ATTACHMENT | SAMPLED`.
- Create resolve targets: one per color, single-sample, same format.

### cleanup() — destroy Vulkan resources
- Clear all `shared_ptr<VkImage>`. Vulkan destructions handled by vulkan::Image destructor (which uses VMA cleanup).

### imageWithIndex()
- Return `_gbufferColors[index]` (the MSAA image).

### Helpers para resoluciones
Add public method: `resolve(VkCommandBuffer cmd)` that blits each MSAA color attachment to its corresponding single-sample resolve target.

```cpp
void GpuAttachmentBuffer::resolve(VkCommandBuffer cmd) {
    for (size_t i = 0; i < _gbufferColors.size(); ++i) {
        vulkan::Image::cmdTransitionImage(
            cmd, _gbufferColors[i]->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );
        vulkan::Image::cmdTransitionImage(
            cmd, _resolveTargets[i]->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        // vkCmdCopyImage — blit MSAA resolve (linear filtering)
        _resolveTargets[i]->cmdCopy(_gbufferColors[i].get());  // reuse existing helper
        vulkan::Image::cmdTransitionImage(
            cmd, _gbufferColors[i]->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        vulkan::Image::cmdTransitionImage(
            cmd, _resolveTargets[i]->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    }
}
```

---

## Relación con la clase base `ColorAttachments`

| Feature | ColorAttachments (existente) | GpuAttachmentBuffer (nuevo) |
|---------|------------------------------|-----------------------------|
| Color attachments format vector | Sí (vector<VkFormat>) | Hardcoded en const. array o std::array< format, 3> |
| MSAA support | No (single-sample) | Sí (4x en color attachments, single-sample en depth) |
| Depth buffer | No tiene depth | Sí (single-sample depth prepass) |
| Resolve targets | No → G-buffer resolve en compositing | Sí (single-sample images para read por compositor) |
| Pipeline layout compatible | Usa solo 1 color attachment | Puede usar hasta 3+1 en dynamic rendering (VkPipelineRenderingCreateInfo ya soporta vector de formats) |

No hay que modificar `ColorAttachments` (ya existe y se usa por `ColorAttachmentsCanvas`). Es una clase independiente con responsabilidades distintas.

---

## Checklist de la fase 1

- [ ] `GpuAttachmentBuffer::build()` crea todas las imágenes con los formatos correctos
- [ ] MSAA 4x aplicado a color attachments y resolución correcta a single-sample targets
- [ ] Depth buffer single sample para depth prepass
- [ ] `GpuAttachmentBuffer::resolve()` realiza blit de MSAA a resolve targets correctamente
- [ ] Transiciones de layout correctas: COLOR_ATTACHMENT_OPTIMAL → ... → SHADER_READ_ONLY_OPTIMAL
- [ ] `GpuAttachmentBuffer::cleanup()` libera todos los recursos Vulkan/VMA correctamente
