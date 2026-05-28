# RT Reflections Implementation Plan

## Overview

Incremental implementation plan for adding ray-traced reflections to the bg2e deferred renderer using Vulkan ray tracing pipelines (`vkCmdTraceRaysKHR`), not compute shaders.

## Key Architectural Findings

1. **No RT pipeline infrastructure exists.** The codebase has acceleration structures (BLAS/TLAS) and ray queries in compute/fragment shaders, but no `RayTracingPipeline` factory, no SBT management, and no `.rgen`/`.rmiss`/`.rchit` shaders.

2. **Extension functions partially loaded.** `vkCreateRayTracingPipelinesKHR` and `vkCmdTraceRaysKHR` are loaded, but `vkGetRayTracingShaderGroupHandlesKHR` (needed for SBT) is missing.

3. **Shader compilation needs update.** `compile_shaders` uses `glslang -V` targeting Vulkan 1.0 / SPIR-V 1.0. RT pipeline shaders require Vulkan 1.2 / SPIR-V 1.4.

4. **TemporalAccumulator is scalar-only.** Hardcodes `VK_FORMAT_R16_SFLOAT`. Reflections need `VK_FORMAT_R16G16B16A16_SFLOAT`.

5. **Composite pass has two variants.** Standard (7 bindings) and RT (8 bindings). Need a third variant with 9 bindings (AO + reflections).

## Implementation Steps

Each step leaves the engine in a compilable state.

| Step | Title | New Files | Modified Files |
|------|-------|-----------|----------------|
| 1 | Extension + Factory | `factory/RayTracingPipeline.hpp`, `.cpp` | `extensions.hpp`, `extensions.cpp` |
| 2 | Shader Compilation | — | `cmake/utils.cmake` |
| 3 | RT Reflection Shaders | `rt_reflections.{rgen,rmiss,rchit}.glsl` | — |
| 4 | RTReflections Class | `deferred/RTReflections.hpp`, `.cpp` | — |
| 5 | RGBA Temporal Accumulator | — | `TemporalAccumulator.hpp`, `.cpp` |
| 6 | Render Flow Integration | — | `DeferredLayer.hpp`, `.cpp` |
| 7 | Composite Integration | — | `DeferredLayer.cpp`, `deferred_composite_rt_reflections.frag.glsl` |
| 8 | Debug Visualization | — | `DeferredLayer.hpp`, `.cpp` |
| 9 | Public API + Docs | — | `RendererDeferred.hpp`, `.cpp` |

## Expected Final Architecture

```
RendererDeferred
  └── SkyboxLayer
  └── DeferredLayer Opaque
        ├── GBufferManager
        ├── RTAmbientOcclusion (compute + ray queries)
        ├── TemporalAccumulator for AO (R16_SFLOAT)
        ├── DenoiseFilter for AO
        ├── RTReflections (RT pipeline + vkCmdTraceRaysKHR)       ← NEW
        ├── TemporalAccumulator for reflections (R16G16B16A16)    ← NEW
        └── Composite pass (PBR + IBL + AO + reflections)         ← MODIFIED
  └── DeferredLayer Transparent
        └── existing transparent composite path
```

## RT Reflections Data Flow

```
G-buffer (depth, normal, material, albedo, fresnel)
    │
    ▼
RTReflections (vkCmdTraceRaysKHR)
    │  Output: VK_FORMAT_R16G16B16A16_SFLOAT
    │  RGB = reflection radiance, A = validity mask
    ▼
TemporalAccumulator (RGBA, ping-pong A/B)
    │  Reprojection + depth/normal validation
    ▼
Composite Pass
    │  mix(envReflection, rtReflection.rgb, rtReflection.a)
    ▼
Final image
```
