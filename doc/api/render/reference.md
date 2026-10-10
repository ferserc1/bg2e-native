# Rendering API Reference

UV and lightmap-related public symbols. The baker and G-buffer types are
available through `<bg2e/render/all.hpp>`; the lighting-pass entry points are
declared in their explicit deferred headers:

| Symbol | Header | Documentation |
|--------|--------|---------------|
| `BakerContext` | `render/BakerContext.hpp` | [BakerContext](BakerContext.md) |
| `GBufferManager`, `GBufferManager::Configuration` | `render/gbuffer/GBufferManager.hpp` | [GBufferManager](GBufferManager.md) |
| `deferred::RTAmbientOcclusion`, `deferred::RTGlobalIllumination` UV methods | `render/deferred/*.hpp` | [UV lighting entry points](UvLightingPasses.md) |
| `UvMapPreviewRenderer` | `render/UvMapPreviewRenderer.hpp` | [UvMapPreviewRenderer](UvMapPreviewRenderer.md) |
| `LightmapMode`, `LightmapPixelFormat`, `LightmapSettings` | `render/LightmapSettings.hpp` | [LightmapSettings](LightmapSettings.md) |
| `LightmapPixels`, `LightmapBaker` | `render/LightmapBaker.hpp` | [LightmapBaker](LightmapBaker.md) |
| `IntegratedBakerContext`, `IntegratedLightmapBaker` | `render/IntegratedBakerContext.hpp` | [IntegratedBakerContext](IntegratedBakerContext.md) |
| `StandaloneBakerContext`, `StandaloneLightmapBaker` | `render/StandaloneBakerContext.hpp` | [Standalone Lightmap Baking](StandaloneBakerContext.md) |
| `StandaloneBakeSceneAssembler` | `render/StandaloneBakeSceneAssembler.hpp` | [StandaloneBakeSceneAssembler](StandaloneBakeSceneAssembler.md) |
| `StandaloneBakeBatch` | `render/StandaloneBakeBatch.hpp` | [StandaloneBakeBatch](StandaloneBakeBatch.md) |

These facilities use production `bg2e::render` Vulkan resources. They are
separate from the experimental `bg2e::gpu` abstraction.

Internal implementation classes `UvSurfacePass`, `UvTemporalAccumulator`,
`LightmapCompositionPass` and `LightmapBakeExecutor` are documented in the
[lightmap architecture guide](../../architecture/lightmaps/index.md); they are
not public API headers.
