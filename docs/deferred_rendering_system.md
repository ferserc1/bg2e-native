# Sistema de Capas de Deferred Rendering

## Tabla de Contenidos

1. [Arquitectura de Clases](#1-arquitectura-de-clases)
2. [G-Buffer: Buffers y Formatos](#2-g-buffer-buffers-y-formatos)
3. [Pipeline de Renderizado](#3-pipeline-de-renderizado-por-frame)
4. [RT Ambient Occlusion](#4-rt-ambient-occlusion)
5. [Temporal Accumulation](#5-temporal-accumulation)
6. [Denoise Filter](#6-denoise-filter)
7. [Composición Final](#7-composición-final)
8. [Gestión Multi-Buffer (Frame In-Flight)](#8-gestión-multi-buffer-frame-in-flight)
9. [Pipelines](#9-pipelines)
10. [Capa Transparente](#10-capa-transparente)
11. [Debug Visualization](#11-debug-visualization)
12. [Shaders](#12-shaders)
13. [Flujo de Datos de Composición](#13-flujo-de-datos-de-composición)

---

## 1. Arquitectura de Clases

```
Renderer (abstracto)
  └── RendererDeferred
        │
        ├── SkyboxLayer          → renderiza cielo a _skyboxImage
        ├── DeferredLayer (Opaque) → G-buffer → RTAO → temporal → denoise → composite a _opaqueImage
        └── DeferredLayer (Transparent) → G-buffer (sin modificar depth) → composite a swapchain
```

### Clases Principales

| Clase | Header | Implementation | Responsabilidad |
|-------|--------|----------------|-----------------|
| `RendererDeferred` | `RendererDeferred.hpp` | `RendererDeferred.cpp` | Orquestador: gestiona 3 capas e imágenes intermedias |
| `RenderLayer` | `deferred/RenderLayer.hpp` | `deferred/RenderLayer.cpp` | Base abstracta para capas (build/render/resize/cleanup) |
| `SkyboxLayer` | `deferred/SkyboxLayer.hpp` | `deferred/SkyboxLayer.cpp` | Renderiza skybox como fondo |
| `DeferredLayer` | `deferred/DeferredLayer.hpp` | `deferred/DeferredLayer.cpp` | Pipeline principal: G-buffer + AO + composición |
| `GBufferManager` | `gbuffer/GBufferManager.hpp` | `gbuffer/GBufferManager.cpp` | Gestiona 5 texturas color + 1 depth para G-buffer |
| `RTAmbientOcclusion` | `deferred/RTAmbientOcclusion.hpp` | `deferred/RTAmbientOcclusion.cpp` | Compute shader para AO vía ray tracing |
| `TemporalAccumulator` | `deferred/TemporalAccumulator.hpp` | `deferred/TemporalAccumulator.cpp` | Acumulación temporal con ping-pong de history |
| `DenoiseFilter` | `deferred/DenoiseFilter.hpp` | `deferred/DenoiseFilter.cpp` | Filtro bilateral denoiser sobre AO acumulada |

---

## 2. G-Buffer: Buffers y Formatos

`GBufferManager` crea 5 texturas color + 1 depth por cada frame in-flight.

### Formatos de Texturas Color

| Binding | Formato | Canales | Contenido |
|---------|---------|---------|-----------|
| 0 - Albedo | `VK_FORMAT_R8G8B8A8_UNORM` | R,G,B,A | Color difuso (RGB) + unused (A) |
| 1 - Normal | `VK_FORMAT_R16G16B16A16_SFLOAT` | R,G,B,A | Normales world-space (XYZ) + unused (A) |
| 2 - Material | `VK_FORMAT_R8G8B8A8_UNORM` | R,G,B,A | Metalness(R), Roughness(G), AO(B), SheenIntensity(A) |
| 3 - Fresnel | `VK_FORMAT_R8G8B8A8_UNORM` | R,G,B,A | FresnelTint(RGB), MaterialFlags(A) |
| 4 - Sheen | `VK_FORMAT_R8G8B8A8_UNORM` | R,G,B,A | SheenColor(RGB) + reserved(A) |

### Formato de Depth

| Buffer | Formato | Descripción |
|--------|---------|-------------|
| Depth | `VK_FORMAT_D32_SFLOAT` | Profundidad simple sin stencil |

### Flags de Uso

Todos los colores usan: `COLOR_ATTACHMENT | SAMPLED | TRANSFER_SRC | TRANSFER_DST`.
Depth usa: `DEPTH_STENCIL_ATTACHMENT | SAMPLED | TRANSFER_SRC | TRANSFER_DST`.

Sin multisampling (`VK_SAMPLE_COUNT_1_BIT`) en ninguno de los buffers.

---

## 3. Pipeline de Renderizado (por frame)

El `RendererDeferred::draw()` ejecuta esta secuencia:

```
[1] SkyboxLayer → _skyboxImage

[2] DeferredLayer (Opaque):
    [2a] G-buffer pass (geometría opaca a 5 texturas color + depth)
    [2b] RTAO compute (si RT soportado) → aoImage
    [2c] Temporal accumulation → accumulatedAO
    [2d] Denoise filter → denoisedAO
    [2e] Composite pass (PBR lighting con AO denoisada) → _opaqueImage

[3] DeferredLayer (Transparent):
    [3a] Copia depth buffer opaco → depth buffer transparente (sin modificar)
    [3b] G-buffer pass (transparent + solid transparent, depth test disabled)
    [3c] Composite pass (usa AO del opaque layer) → swapchain

[4] GizmoAndSelectionRenderer overlay (si no offscreen)
```

### Detalle del Composite Pass

El composite pass lee G-buffers + input image (skybox u opaque previo) + depth + denoised AO (si RT) y aplica:

- PBR forward lighting con hasta 8 luces
- IBL (irradiance map + prefiltered envmap + BRDF LUT)
- Tonemapping (gamma 2.2)
- Corrección de color (brightness, contrast, exposure)

---

## 4. RT Ambient Occlusion

Compute shader (`rt_ao.comp.glsl`) que lanza rayos desde la TLAS contra la geometría de la escena para calcular oclusión ambiental.

### Parámetros Configurables

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| sampleCount | 8 | Rayos por pixel |
| bounceCount | 2 | Rebotes de rayos |
| radius | 0.56 | Radio de búsqueda de rayos |
| bias | 0.017 | Sesgo anti-false-positivos |
| falloff | 1.0 | Factor de atenuación por distancia |
| bounceAttenuation | 0.35 | Atenuación por rebote |

### Niveles de Calidad

| Quality | Resolution Scale |
|---------|-----------------|
| Ultra | 1.0x (resolución completa) |
| High | 2/3x |
| Medium | 0.5x |
| Low | 1/3x |

### Push Constants (AOPushConstants)

```cpp
struct AOPushConstants {
    glm::mat4 inverseViewProjection;  // VP invertida para reconstrucción de posición
    int sampleCount;
    int bounceCount;
    float radius;
    float bias;
    float falloff;
    float bounceAttenuation;
    uint32_t frameIndex;
    int padding0;
};
```

### Output

Textura individual de AO por frame in-flight, accesible vía `_rtAmbientOcclusion->aoImage(frameIndex)`.

---

## 5. Temporal Accumulation

### Técnica

Re-proyección temporal con ping-pong para suavizar RTAO frame a frame, permitiendo usar menos rayos por frame manteniendo calidad visual.

### Buffers (por frame in-flight)

| Buffer | Tipo | Cantidad |
|--------|------|----------|
| `_historyImagesA` | `vector<shared_ptr<vulkan::Image>>` | `numImages()` |
| `_historyImagesB` | `vector<shared_ptr<vulkan::Image>>` | `numImages()` |
| `_prevDepthImages` | `vector<shared_ptr<vulkan::Image>>` | `numImages()` |
| `_prevNormalImages` | `vector<shared_ptr<vulkan::Image>>` | `numImages()` |

### Mecanismo de Ping-Pong

1. Cada frame tiene un `_writeIndex` que alterna entre A y B
2. Read index = `1 - writeIndex` (historia del frame anterior)
3. El shader de acumulación lee del read buffer y escribe en el write buffer

### Validación de Alineación

Para evitar ghosting cuando los objetos se mueven:

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| depthThreshold | 0.01 | Descarta pixeles donde la profundidad cambió mucho |
| normalThreshold | 0.8 | Descarta pixeles donde la normal cambió (dot product) |

### Weight Blending

- **Default:** 90% historia + 10% current (configurable vía `setHistoryWeight`)
- El weight se pasa en push constants como `historyWeight`

### Modes de Acumulación

| Mode | Descripción |
|------|-------------|
| `Progressive` | Acumula desde frame 0, sin período de warm-up. Mejor calidad cinematográfica |
| `Interactive` | Con período de warm-up. Mejor para interacción en tiempo real |

### Invalidez de Historia

Si la cámara cambia más de epsilon 0.001f entre frames, se invalida la historia temporal para evitar artefactos de desalineación.

### Push Constants (AccumulatorPushConstants)

```cpp
struct AccumulatorPushConstants {
    glm::mat4 currentInverseViewProjection;
    glm::mat4 previousViewProjection;
    glm::vec2 outputSize;
    float historyWeight;
    uint32_t accumulatedFrameCount;
    uint32_t useProgressiveMode;
    uint32_t hasHistory;
    float depthThreshold;
    float normalThreshold;
};
```

---

## 6. Denoise Filter

Filtro bilateral compute (`denoise_bilateral.comp.glsl`) sobre la AO temporalmente acumulada.

### Parámetros

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| kernelRadius | 2 | Radio del kernel bilateral |
| depthThreshold | 0.01 | Umbral de profundidad para preservar bordes |
| normalThreshold | 0.8 | Umbral de normal para preservar bordes |
| depthSigma | 0.01 | Sigma del canal de profundidad |
| normalSigma | 0.3 | Sigma del canal de normales |

### Funcionamiento

El filtro usa el G-buffer como guía (guid image) para preservar bordes durante el suavizado:

1. Lee la AO temporalmente acumulada
2. Lee depth y normals del G-buffer
3. Aplica filtro bilateral ponderado por similitud de profundidad y normales
4. Escribe el resultado en el output buffer

### Push Constants (DenoisePushConstants)

```cpp
struct DenoisePushConstants {
    glm::vec2 outputSize;
    int kernelRadius;
    float depthThreshold;
    float normalThreshold;
    float depthSigma;
    float normalSigma;
    uint32_t padding;
};
```

### Output

Textura de AO denoised por frame in-flight, accesible vía `_denoiseFilter->outputImage(frameIndex)`.

---

## 7. Composición Final

### Descriptor Sets del Composite Pass

#### Standard (sin RT) — 7 bindings

| Binding | Tipo | Contenido |
|---------|------|-----------|
| 0 | `COMBINED_IMAGE_SAMPLER` | g_Albedo (G-buffer color 0) |
| 1 | `COMBINED_IMAGE_SAMPLER` | g_Normal (G-buffer color 1) |
| 2 | `COMBINED_IMAGE_SAMPLER` | g_Material (G-buffer color 2) |
| 3 | `COMBINED_IMAGE_SAMPLER` | g_FresnelFlags (G-buffer color 3) |
| 4 | `COMBINED_IMAGE_SAMPLER` | g_SheenColor (G-buffer color 4) |
| 5 | `COMBINED_IMAGE_SAMPLER` | g_InputImage (skybox u opaque previo) |
| 6 | `COMBINED_IMAGE_SAMPLER` | g_Depth (G-buffer depth) |

#### RT-enhanced — 8 bindings (agrega)

| Binding | Tipo | Contenido |
|---------|------|-----------|
| 7 | `COMBINED_IMAGE_SAMPLER` | g_AO (AO denoisada) |

#### Layouts de Pipeline

```
Composite (sin RT):
  Set 0: G-buffer descriptor set (7 samplers)
  Set 1: Fragment frame data (view/proj)
  Set 2: Environment data (IBL + lights)
  Set 3: Light data binding

Composite RT:
  Set 0: G-buffer descriptor set (8 samplers)
  Set 1: Fragment frame data (view/proj)
  Set 2: Environment data (IBL + lights)
  Set 3: Light data binding
  Set 4: Ray tracing scene data (TLAS)
```

### Push Constants (CompositePushConstants)

```cpp
struct CompositePushConstants {
    float gamma;                    // 2.2f
    float brightness;               // configurable
    float contrast;                 // configurable
    float exposure;                 // configurable
    uint32_t numLights;             // cantidad de luces activas
    uint32_t padding1;
    uint32_t padding2;
    uint32_t padding3;
    glm::mat4 inverseViewProjection;
};
```

---

## 8. Gestión Multi-Buffer (Frame In-Flight)

### Estrategia

Cada buffer se replica `numImages()` veces (típicamente 3 frames in-flight). La selección se hace vía `_engine->currentFrameResourcesIndex()`.

### Tabla de Buffers por Frame In-Flight

| Buffer | Tipo | Size | Acceso |
|--------|------|------|--------|
| `_gbuffers` | `vector<unique_ptr<GBufferManager>>` | `numImages()` | `_gbuffers[frameIndex]` |
| `_aoImages` (RTAO) | `vector<shared_ptr<vulkan::Image>>` | `numImages()` | `aoImage(frameIndex)` |
| `_historyImagesA` (temporal) | `vector<shared_ptr<vulkan::Image>>` | `numImages()` | `historyImagesA[frameIndex]` |
| `_historyImagesB` (temporal) | `vector<shared_ptr<vulkan::Image>>` | `numImages()` | `historyImagesB[frameIndex]` |
| `_prevDepthImages` (temporal) | `vector<shared_ptr<vulkan::Image>>` | `numImages()` | `prevDepthImages[frameIndex]` |
| `_prevNormalImages` (temporal) | `vector<shared_ptr<vulkan::Image>>` | `numImages()` | `prevNormalImages[frameIndex]` |
| `_outputImages` (denoise) | `vector<shared_ptr<vulkan::Image>>` | `numImages()` | `outputImage(frameIndex)` |

### Data Bindings por Frame

| Binding | Tipo | Contenido |
|---------|------|-----------|
| `_frameDataBinding` | `scene::vk::FrameDataBinding` | Scene data (view/proj matrices) |
| `_fragmentFrameDataBinding` | `scene::vk::FrameDataBinding` | View/proj para el composite pass |
| `_objectDataBinding` | `scene::vk::ObjectDataBinding` | Model transform + material por objeto |
| `_environmentDataBinding` | `scene::vk::EnvironmentDataBinding` | Lights + IBL (irradiance, prefiltered envmap, BRDF LUT) |
| `_lightDataBinding` | `scene::vk::DeferredLightDataBinding` | Lights específicas de deferred |
| `_rtDataBinding` | `vulkan::rt::RayTracingSceneDataBinding` | TLAS acceleration structure |

### Gestión de Descriptor Sets

Los descriptor sets se crean dinámicamente en cada frame vía `frameResources.newDescriptorSet()`, no se pre-asignan. Esto permite reutilizar la memoria de descriptor sets entre frames de forma segura.

---

## 9. Pipelines

### Tabla de Pipelines

| Pipeline | Vertex Shader | Fragment Shader | Stage | Descripción |
|----------|--------------|-----------------|-------|-------------|
| GBuffer | `deferred_gbuffer.vert.spv` | `deferred_gbuffer.frag.spv` | Graphics | Renderiza geometría al G-buffer |
| Composite (standard) | `deferred_composite.vert.spv` | `deferred_composite.frag.spv` | Graphics | PBR lighting sin RT AO |
| Composite RT | `deferred_composite.vert.spv` | `deferred_composite_rt.frag.spv` | Graphics | PBR lighting con AO denoised |
| Debug Blit | `deferred_debug_blit.vert.spv` | `deferred_debug_blit.frag.spv` | Graphics | Debug visualization fullscreen quad |

### Características de los Pipelines

| Pipeline | Depth Test | Cull Mode | Multisample | Vertex Input | Topology |
|----------|-----------|-----------|-------------|--------------|----------|
| GBuffer | `VK_COMPARE_OP_LESS` (disabled en transparente) | `VK_CULL_MODE_BACK_BIT` | Disabled | `scene::Drawable` | `TRIANGLE_LIST` |
| Composite | Disabled | `VK_CULL_MODE_BACK_BIT` | Disabled | None (fullscreen quad) | `TRIANGLE_LIST` |
| Composite RT | Disabled | `VK_CULL_MODE_BACK_BIT` | Disabled | None (fullscreen quad) | `TRIANGLE_LIST` |
| Debug Blit | Disabled | `VK_CULL_MODE_BACK_BIT` | Disabled | None (fullscreen quad) | `TRIANGLE_LIST` |

### Pipeline GBuffer

```cpp
// Pipeline layout
Set 0: Frame data binding (view/proj)
Set 1: Object data binding (model + material + 5 texture samplers)
Push constants: CompositePushConstants (gamma, brightness, contrast, exposure, numLights, inverseViewProjection)
```

### Pipeline Composite

```cpp
// Pipeline layout (sin RT)
Set 0: G-buffer descriptor set (7 samplers)
Set 1: Fragment frame data binding
Set 2: Environment data binding
Set 3: Light data binding
Push constants: CompositePushConstants

// Pipeline layout (con RT)
Set 0: G-buffer descriptor set (8 samplers)
Set 1: Fragment frame data binding
Set 2: Environment data binding
Set 3: Light data binding
Set 4: Ray tracing scene data binding (TLAS)
Push constants: CompositePushConstants
```

### Selección de Pipeline en Runtime

```cpp
bool useRT = _useRtShadows && tlas != VK_NULL_HANDLE;
VkPipeline activePipeline = useRT ? _compositePipelineRT : _compositePipeline;
VkPipelineLayout activeLayout = useRT ? _compositePipelineRTLayout : _compositePipelineLayout;
```

---

## 10. Capa Transparente

La capa transparente (`LayerType::Transparent`) tiene tratamiento especial respecto a la opaca:

### Diferencias Clave

| Aspecto | Opaque | Transparent |
|---------|--------|-------------|
| Depth test | Enabled (`VK_COMPARE_OP_LESS`) | Disabled |
| Depth write | Enabled | Disabled |
| Depth load op | `VK_ATTACHMENT_LOAD_OP_CLEAR` | `VK_ATTACHMENT_LOAD_OP_LOAD` |
| Render queue | `Opaque` | `Transparent` + `SolidTransparent` |
| Input image | `_skyboxImage` | `_opaqueImage` |
| Depth buffer | Propio | Copia del depth buffer opaco |

### Mecanismo de Copia de Depth

Antes del render de la capa transparente, se copia el depth buffer del opaque layer:

```cpp
// 1. Transicionar ambos depth buffers a layouts de transfer
// 2. vkCmdCopyImage desde opaqueDepthBuffer → transparentDepthBuffer
// 3. Transicionar a SHADER_READ_ONLY_OPTIMAL
```

Esto preserva la oclusión de objetos opacos sin que los transparentes modifiquen el depth buffer.

### Flujo de la Capa Transparente

```
[3a] Copiar depth buffer opaco → depth buffer transparente
[3b] G-buffer pass (transparent + solid transparent, sin modificar depth)
[3c] Composite pass (lee G-buffers + _opaqueImage como input + AO del opaque layer) → swapchain
```

---

## 11. Debug Visualization

El `DeferredDebugVisualization` enum permite 11 modos de depuración:

| Mode | Buffer Visualizado |
|------|-------------------|
| `FullComposition` | Resultado final de composición PBR |
| `GBufferAlbedo` | Textura albedo del G-buffer (binding 0) |
| `GBufferNormal` | Textura normals del G-buffer (binding 1) |
| `GBufferMaterial` | Textura materials del G-buffer (binding 2) |
| `GBufferFresnelFlags` | Textura fresnel del G-buffer (binding 3) |
| `GBufferSheenColor` | Textura sheen del G-buffer (binding 4) |
| `GBufferDepth` | Depth buffer del G-buffer |
| `InputImage` | Imagen de entrada (skybox u opaque) |
| `RTAmbientOcclusion` | AO raw del compute shader RT |
| `DenoisedAO` | AO tras el filtro denoise |
| `TemporalAccumulatedAO` | AO tras la acumulación temporal |

### Implementación

Se usa un pipeline de debug blit (`deferred_debug_blit`) que renderiza un fullscreen quad con la textura correspondiente. El método `resolveDebugSource()` mapea cada modo al buffer correcto.

---

## 12. Shaders

### Archivos de Shader

| Shader | Path | Stage | Propósito |
|--------|------|-------|-----------|
| `deferred_gbuffer.vert.glsl` | `shaders/src/` | Vertex | Vertex shader G-buffer (world pos, normals, TBN) |
| `deferred_gbuffer.frag.glsl` | `shaders/src/` | Fragment | Fragment shader G-buffer (5 outputs color) |
| `deferred_composite.vert.glsl` | `shaders/src/` | Vertex | Fullscreen quad para composite |
| `deferred_composite.frag.glsl` | `shaders/src/` | Fragment | PBR forward lighting sin RT AO |
| `deferred_composite_rt.frag.glsl` | `shaders/src/` | Fragment | PBR forward lighting con AO denoised de RT |
| `deferred_debug_blit.vert.glsl` | `shaders/src/` | Vertex | Debug blit vertex shader |
| `deferred_debug_blit.frag.glsl` | `shaders/src/` | Fragment | Debug blit fragment shader |
| `rt_ao.comp.glsl` | `shaders/src/` | Compute | Ray tracing ambient occlusion |
| `temporal_accumulation.comp.glsl` | `shaders/src/` | Compute | Temporal accumulation ping-pong |
| `denoise_bilateral.comp.glsl` | `shaders/src/` | Compute | Bilateral denoise con G-buffer guides |
| `lib/deferred_utils.glsl` | `shaders/src/lib/` | Include | Utilidades compartidas |

### Includes de Shaders

Los shaders usan `#include "lib/*.glsl"` para funciones compartidas de PBR. Las includes se resuelven por el include path de glslang — no se debe mover `shaders/src/lib/` de posición relativa.

---

## 13. Flujo de Datos de Composición

```
┌─────────────────────────────────────────────────────────────────────┐
│                        RENDERER DEFERRED                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  [1] SKYBOX LAYER                                                   │
│       └─► _skyboxImage                                              │
│                                                                     │
│  [2] OPAQUE LAYER                                                   │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ G-BUFFER PASS                                            │   │
│       │  Input: Opaque geometry from RenderQueue                 │   │
│       │  Output: 5 color textures + depth (G-buffer)             │   │
│       │  Formats: R8G8B8A8_UNORM x4, R16G16B16A16_SFLOAT x1,   │   │
│       │         D32_SFLOAT x1                                   │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ RT AO COMPUTE                                            │   │
│       │  Input: G-buffer (depth, normals, positions) + TLAS      │   │
│       │  Output: aoImage (per frame in-flight)                   │   │
│       │  Shader: rt_ao.comp.glsl                                 │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ TEMPORAL ACCUMULATION                                    │   │
│       │  Input: aoImage + G-buffer (depth, normals) + history    │   │
│       │  Output: accumulatedAO (ping-pong A/B)                   │   │
│       │  Shader: temporal_accumulation.comp.glsl                 │   │
│       │  Weight: 90% history + 10% current (default)             │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ DENOISE FILTER                                           │   │
│       │  Input: accumulatedAO + G-buffer (depth, normals)        │   │
│       │  Output: denoisedAO (per frame in-flight)                │   │
│       │  Shader: denoise_bilateral.comp.glsl                     │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ COMPOSITE PASS                                           │   │
│       │  Input: G-buffers + _skyboxImage + denoisedAO + depth    │   │
│       │  Operations: PBR lighting + IBL + 8 lights + tonemapping │   │
│       │  Output: _opaqueImage                                    │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│  [3] TRANSPARENT LAYER                                              │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ COPY DEPTH                                               │   │
│       │  opaqueDepthBuffer → transparentDepthBuffer              │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ G-BUFFER PASS (transparent)                              │   │
│       │  Input: Transparent + SolidTransparent geometry          │   │
│       │  Depth test: DISABLED                                    │   │
│       │  Output: G-buffer (depth loaded from opaque)             │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│       ┌─────────────────────────────────────────────────────────┐   │
│       │ COMPOSITE PASS (transparent)                             │   │
│       │  Input: G-buffers + _opaqueImage + AO (from opaque)      │   │
│       │  Output: swapchain color image                           │   │
│       └─────────────────────────────────────────────────────────┘   │
│                         │                                           │
│  [4] SELECTION HIGHLIGHT (non-offscreen only)                       │
│       Overlay sobre el color image final                            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Resumen de Formatos

### G-Buffer

| # | Formato | Canales | Uso |
|---|---------|---------|-----|
| 0 | `VK_FORMAT_R8G8B8A8_UNORM` | R=Albedo, G=Albedo, B=Albedo, A=unused | Color material |
| 1 | `VK_FORMAT_R16G16B16A16_SFLOAT` | X=Normal, Y=Normal, Z=Normal, A=unused | Normales world-space |
| 2 | `VK_FORMAT_R8G8B8A8_UNORM` | R=Metalness, G=Roughness, B=AO, A=SheenIntensity | Atributos PBR |
| 3 | `VK_FORMAT_R8G8B8A8_UNORM` | R=FresnelTint, G=FresnelTint, B=FresnelTint, A=MaterialFlags | Fresnel |
| 4 | `VK_FORMAT_R8G8B8A8_UNORM` | R=SheenColor, G=SheenColor, B=SheenColor, A=reserved | Sheen |
| D | `VK_FORMAT_D32_SFLOAT` | Depth | Profundidad |

### Intermedias

| Buffer | Formato | Uso |
|--------|---------|-----|
| `_skyboxImage` | `_colorImageFormat` (swapchain format) | Skybox como fondo |
| `_opaqueImage` | `_colorImageFormat` (swapchain format) | Resultado capa opaca |
| `aoImage` | `R32_SFLOAT` (probable) | AO raw del RT compute |
| `accumulatedAO` | `R32_SFLOAT` (probable) | AO temporalmente acumulada |
| `denoisedAO` | `R32_SFLOAT` (probable) | AO denoised |