# Fase 5: Integración en DeferredLayer.cpp

## Archivo

`lib/src/bg2e/render/deferred/DeferredLayer.cpp`

## Cambios

### 1. build()

Después de `_rtAmbientOcclusion->build(extent);`, añadir:

```cpp
// Create denoise filter
_denoiseFilter = std::make_unique<DenoiseFilter>(_engine);
_denoiseFilter->build(gbuffer.get(), extent);
```

Donde `gbuffer` es el primer elemento de `_gbuffers` (se puede obtener antes del bucle de creación de G-buffers).

**Código completo modificado:**

```cpp
void DeferredLayer::build(VkExtent2D extent, VkFormat outputFormat)
{
    RenderLayer::build(extent, outputFormat);

    // Create per-frame G-buffer managers
    _gbuffers.resize(_engine->numImages());
    for (auto& gb : _gbuffers)
    {
        gb = std::make_unique<GBufferManager>(_engine);
        gb->build(extent);
    }

    // Create AO pass
    _rtAmbientOcclusion = std::make_unique<RTAmbientOcclusion>(_engine);
    _rtAmbientOcclusion->build(extent);

    // NUEVO: Create denoise filter
    _denoiseFilter = std::make_unique<DenoiseFilter>(_engine);
    _denoiseFilter->build(_gbuffers[0].get(), extent);

    // ... (resto sin cambios: data bindings, pipelines, sampler)
}
```

### 2. render()

Después del bloque de RTAO (línea ~216), añadir el denoise pass:

```cpp
// NUEVO: Denoise pass
{
    auto aoImg = _rtAmbientOcclusion->aoImage(frameResourcesIndex);
    _denoiseFilter->render(cmd, currentFrame, frameResources, gbuffer, aoImg.get());
}
```

**Código completo modificado (bloque AO + denoise):**

```cpp
// AO pass: compute ambient occlusion from G-buffers + TLAS
{
    auto projMat = _scene->mainCamera()->projectionMatrix();
    auto viewMat = _scene->mainCamera()->viewMatrix();
    auto invVP = glm::inverse(projMat * viewMat);
    _rtAmbientOcclusion->render(cmd, currentFrame, frameResources, gbuffer, invVP);
}

// NUEVO: Denoise pass - filter the AO image
{
    auto aoImg = _rtAmbientOcclusion->aoImage(frameResourcesIndex);
    _denoiseFilter->render(cmd, currentFrame, frameResources, gbuffer, aoImg.get());
}
```

### 3. renderCompositePass()

Cambiar la línea que añade la imagen AO al descriptor set (línea ~561-563):

**Antes:**

```cpp
auto aoImg = _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex());
gbufferDS->addImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    aoImg.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
```

**Después:**

```cpp
auto denoisedAoImg = _denoiseFilter->outputImage(_engine->currentFrameResourcesIndex());
gbufferDS->addImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    denoisedAoImg.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
```

Esto hace que el composite shader muestree la imagen filtrada en lugar de la AO sin filtrar.

### 4. resize()

Añadir después de `_rtAmbientOcclusion->resize(newExtent);`:

```cpp
_denoiseFilter->resize(newExtent);
```

### 5. cleanup()

Añadir después de `_rtAmbientOcclusion->cleanup();`:

```cpp
if (_denoiseFilter) _denoiseFilter->cleanup();
```

### 6. resolveDebugSource() (opcional)

Añadir un nuevo caso al switch para debug visualización:

```cpp
case DeferredDebugVisualization::DenoisedAO:
    return _denoiseFilter->outputImage(_engine->currentFrameResourcesIndex()).get();
```

Y añadir `DenoisedAO` al enum en el header:

```cpp
enum class DeferredDebugVisualization {
    FullComposition = 0,
    GBufferAlbedo,
    GBufferNormal,
    GBufferMaterial,
    GBufferFresnelFlags,
    GBufferSheenColor,
    GBufferDepth,
    InputImage,
    RTAmbientOcclusion,
    DenoisedAO,        // NUEVO

    MaxLayer
};
```

## Resumen de modificaciones

| Función | Cambio |
|---------|--------|
| `build()` | Crear y construir `_denoiseFilter` |
| `render()` | Llamar `_denoiseFilter->render()` después de RTAO |
| `renderCompositePass()` | Usar `_denoiseFilter->outputImage()` en binding 7 |
| `resize()` | Llamar `_denoiseFilter->resize()` |
| `cleanup()` | Llamar `_denoiseFilter->cleanup()` |
| `resolveDebugSource()` | (opcional) Añadir caso `DenoisedAO` |

## Barreras

Las barreras están completamente gestionadas dentro de `DenoiseFilter::render()`:

1. **RTAO → Denoise**: RTAO deja la AO en `SHADER_READ_ONLY_OPTIMAL`. Denoise la lee como `sampler2D` en ese layout — sin transición necesaria.
2. **Denoise → Composite**: Denoise deja la salida en `SHADER_READ_ONLY_OPTIMAL`. Composite la lee como `sampler2D` en ese layout — sin transición necesaria.

Todo es transparente para DeferredLayer.