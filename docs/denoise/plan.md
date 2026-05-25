# Plan de Implementación: Filtro Denoise (Cross-Bilateral Blur)

## Objetivo

Implementar un filtro de denoising mediante la técnica **depth/normal-aware cross-bilateral blur** que se aplique a la imagen de ambient occlusion generada por RTAmbientOcclusion. El filtro es genérico y no depende de ray tracing — podría utilizarse en el futuro para filtrar cualquier imagen.

## Resumen de Fases

| Fase | Archivo(s) | Descripción |
|------|-----------|-------------|
| Fase 1 | `shaders/src/denoise_bilateral.comp.glsl` | Compute shader GLSL con el algoritmo bilateral |
| Fase 2 | `lib/include/bg2e/render/deferred/DenoiseFilter.hpp` | Clase C++ — interfaz y estructura de push constants |
| Fase 3 | `lib/src/bg2e/render/deferred/DenoiseFilter.cpp` | Clase C++ — implementación (pipeline, descriptors, render) |
| Fase 4 | `lib/include/bg2e/render/deferred/DeferredLayer.hpp` | Integración — añadir miembro `_denoiseFilter` |
| Fase 5 | `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | Integración — build, render, resize, cleanup, composite pass |

## Archivos Nuevos

- `shaders/src/denoise_bilateral.comp.glsl` — Compute shader
- `lib/include/bg2e/render/deferred/DenoiseFilter.hpp` — Header de la clase
- `lib/src/bg2e/render/deferred/DenoiseFilter.cpp` — Implementación

## Archivos Modificados

- `lib/include/bg2e/render/deferred/DeferredLayer.hpp`
- `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

## CMake

No se necesitan cambios — el sistema de build incluye automáticamente todos los `.glsl` y `.cpp` de sus directorios respectivos.

## Patrón de Diseño

La clase `DenoiseFilter` sigue el mismo patrón que `RTAmbientOcclusion`:
- Constructor recibe `Engine*`
- `build()` crea sampler, imágenes por-frame, pipeline y layouts
- `render()` maneja transiciones de layout, descriptor sets, push constants y dispatch
- `resize()` recrea las imágenes
- `cleanup()` destruye todos los recursos Vulkan
- Imágenes por-frame con `std::vector<std::shared_ptr<vulkan::Image>>`
- Push constants para parámetros modificables en tiempo real

## Barreras de Sincronización

Todas las barreras se gestionan de forma transparente dentro de `DenoiseFilter::render()`:

1. **Input**: La imagen AO llega en `SHADER_READ_ONLY_OPTIMAL` (establecido por RTAmbientOcclusion). El shader la lee como `sampler2D` — no necesita transición.
2. **Output**: La imagen de resultado transiciona `UNDEFINED → GENERAL` antes del dispatch, y `GENERAL → SHADER_READ_ONLY_OPTIMAL` después.

El composite pass recibe la imagen filtrada en `SHADER_READ_ONLY_OPTIMAL`, listo para ser muestreado.

## Parámetros del Filtro (Push Constants)

| Parámetro | Tipo | Descripción |
|-----------|------|-------------|
| `outputSize` | `vec2` | Dimensiones de la imagen de salida (para UV) |
| `kernelRadius` | `int` | Radio del kernel de blur (e.g. 3) |
| `depthThreshold` | `float` | Umbral de diferencia de profundidad para rechazo de bordes |
| `normalThreshold` | `float` | Umbral de producto escalar de normales para rechazo de bordes |
| `depthSigma` | `float` | Desviación estándar para ponderación de profundidad |
| `normalSigma` | `float` | Desviación estándar para ponderación de normales |

## Shader: Cross-Bilateral Blur

El algoritmo muestrea la imagen de entrada y los g-buffers (depth + normal) para cada píxel del kernel, calculando un peso bilateral:

```
weight = spatial_gaussian * depth_weight * normal_weight

depth_weight  = exp(-|depth_c - depth_n|^2 / (2 * depthSigma^2))
normal_weight = exp(-(1 - dot(normal_c, normal_n)) / normalSigma)
```

Los píxeles con profundidad y normal similares tienen más peso, preservando los bordes de la escena.

## Integración en DeferredLayer

- Se crea una instancia de `DenoiseFilter` por cada frame resource (junto con `_rtAmbientOcclusion` y `_gbuffers`)
- Después del render de RTAO, se ejecuta el denoise sobre la imagen AO
- El composite pass muestrea `_denoiseFilter->outputImage(frameIndex)` en lugar de `_rtAmbientOcclusion->aoImage(frameIndex)` para el binding `g_AO`