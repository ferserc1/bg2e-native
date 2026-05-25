# Fase 4: Integración en DeferredLayer.hpp

## Archivo

`lib/include/bg2e/render/deferred/DeferredLayer.hpp`

## Cambios

### Include

Añadir al inicio del fichero:

```cpp
#include <bg2e/render/deferred/DenoiseFilter.hpp>
```

### Miembro

Añadir al final de los miembros protegidos (después de `_rtAmbientOcclusion`):

```cpp
std::unique_ptr<DenoiseFilter> _denoiseFilter;
```

### Resumen de la estructura resultante

```cpp
class BG2E_API DeferredLayer : public RenderLayer {
public:
    // ... (sin cambios)

protected:
    LayerType _layerType;

    std::vector<std::unique_ptr<GBufferManager>> _gbuffers;
    std::shared_ptr<vulkan::Image> _opaqueDepthBuffer;

    // ... (pipelines, sin cambios)

    std::unique_ptr<RTAmbientOcclusion> _rtAmbientOcclusion;

    // NUEVO:
    std::unique_ptr<DenoiseFilter> _denoiseFilter;

    // ... (CompositePushConstants y funciones, sin cambios)
};
```

## Notas

- No se necesitan cambios en los métodos públicos ni en las declaraciones de funciones existentes
- El miembro `_denoiseFilter` es `std::unique_ptr` como los data bindings existentes
- Se coloca junto a `_rtAmbientOcclusion` porque están conceptualmente relacionados