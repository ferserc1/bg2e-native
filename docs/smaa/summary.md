# SMAA Post-Processing — Implementation Plan Summary

## Objective

Add Subpixel Morphological Anti-Aliasing (SMAA) as a global post-processing stage to the deferred renderer (`bg2e::render::RendererDeferred`). SMAA runs after the final composed LDR image is produced and before the UI/editor overlay, using three compute passes.

## Architecture

```
SMAAProcessor
├── Engine* _engine
├── VkExtent2D _extent
├── VkSampler _sampler
│
├── // Persistent pipelines (one per pass)
├── VkPipeline _edgeDetectionPipeline
├── VkPipeline _blendWeightPipeline
├── VkPipeline _neighborhoodBlendPipeline
├── VkPipelineLayout _edgePipelineLayout
├── VkPipelineLayout _blendPipelineLayout
├── VkPipelineLayout _nblendPipelineLayout
├── VkDescriptorSetLayout _edgeDSLayout
├── VkDescriptorSetLayout _blendWeightDSLayout
├── VkDescriptorSetLayout _neighborhoodBlendDSLayout
│
├── // Static LUT textures (generated once, persistent)
├── vulkan::Image* _areaTexture              (RG8, e.g. 256×256)
├── vulkan::Image* _searchTexture            (R8, e.g. 64×64)
│
├── // Per-frame-in-flight resources
├── vector<FrameData> _frames
│   ├── edgesImage       (RG8_UNORM, viewport size)
│   ├── blendWeightsImage (RGBA8_UNORM, viewport size)
│   └── outputImage       (same format as swapchain colorImage, viewport size)
│
├── SMAAProcessor(Engine* engine)
├── void build(VkExtent2D extent, VkFormat outputFormat = VK_FORMAT_R8G8B8A8_UNORM)
├── void resize(VkExtent2D newExtent)
├── const vulkan::Image* process(VkCommandBuffer cmd, uint32_t frameIndex, const vulkan::Image* inputImage)
├── void cleanup()
```

## Rendering Pipeline Integration

Current flow in `RendererDeferred::draw()`:

```
1. prepareSceneRender()
2. SkyboxLayer → _skyboxImage
3. OpaqueLayer → _opaqueImage
4. TransparentLayer → colorImage (swapchain)
5. GizmoAndSelectionRenderer → colorImage
6. endSceneRender()
7. Transition colorImage → COLOR_ATTACHMENT_OPTIMAL
```

After SMAA integration:

```
1. prepareSceneRender()
2. SkyboxLayer → _skyboxImage
3. OpaqueLayer → _opaqueImage
4. TransparentLayer → colorImage
5. GizmoAndSelectionRenderer → colorImage
6. SMAAProcessor::process(colorImage) → colorImage   ← NEW
7. endSceneRender()
8. Transition colorImage → COLOR_ATTACHMENT_OPTIMAL
```

The UI overlay (`_renderUICallback` in `RenderLoop`) renders after `draw()` returns, so it is unaffected by SMAA.

## SMAA Algorithm (Three Compute Passes)

### Pass 1: Edge Detection

- **Input**: final composed image (colorImage)
- **Output**: `edgesImage` (RG8_UNORM)
- **Algorithm**: Luma-based edge detection. For each pixel, compare luma with neighbors. Write 1.0 to the R channel if horizontal edge detected, G channel if vertical edge detected.

### Pass 2: Blend Weight Calculation

- **Inputs**: `edgesImage`, `areaTexture`, `searchTexture`
- **Output**: `blendWeightsImage` (RGBA8_UNORM)
- **Algorithm**: For each edge pixel, search for edges in the neighborhood using `searchTexture` (search for line endpoints), then look up blend weights from `areaTexture`. Each RGBA channel stores the blend weight for one of the four possible directions (left, top, right, bottom).

### Pass 3: Neighborhood Blending

- **Inputs**: final composed image, `blendWeightsImage`
- **Output**: `outputImage` (same format as input)
- **Algorithm**: For each pixel, compute the weighted average of neighboring pixels using the blend weights. The result is the anti-aliased image.

## LUT Generation

The `areaTexture` and `searchTexture` are static lookup tables that encode SMAA's edge pattern matching data. They are generated once on the first frame using two additional compute shaders:

- `smaa_area_generate.comp.glsl`: Generates the area/weight LUT (256×256, RG8)
- `smaa_search_generate.comp.glsl`: Generates the search distance LUT (64×64, R8)

These are generated via `GPUProcess::executeShader()` (immediate submit) since they only run once and the result is stored permanently.

## Phases

| Phase | Description | Files |
|-------|-------------|-------|
| 1 | Compute shaders | `shaders/src/glsl/smaa_edge_detection.comp.glsl`, `smaa_blend_weight.comp.glsl`, `smaa_neighborhood_blend.comp.glsl`, `smaa_area_generate.comp.glsl`, `smaa_search_generate.comp.glsl` (create) |
| 2 | Class header | `lib/include/bg2e/render/deferred/SMAAProcessor.hpp` (create) |
| 3 | Class implementation | `lib/src/bg2e/render/deferred/SMAAProcessor.cpp` (create) |
| 4 | Renderer integration | `lib/include/bg2e/render/RendererDeferred.hpp`, `lib/src/bg2e/render/RendererDeferred.cpp` (modify) |

## Files Created

- `shaders/src/glsl/smaa_edge_detection.comp.glsl`
- `shaders/src/glsl/smaa_blend_weight.comp.glsl`
- `shaders/src/glsl/smaa_neighborhood_blend.comp.glsl`
- `shaders/src/glsl/smaa_area_generate.comp.glsl`
- `shaders/src/glsl/smaa_search_generate.comp.glsl`
- `lib/include/bg2e/render/deferred/SMAAProcessor.hpp`
- `lib/src/bg2e/render/deferred/SMAAProcessor.cpp`

## Files Modified

- `lib/include/bg2e/render/RendererDeferred.hpp`
- `lib/src/bg2e/render/RendererDeferred.cpp`

## CMake

No CMake changes needed — the build system automatically includes all `.glsl` files from `shaders/src/` and all `.cpp`/`.hpp` files from `lib/src/` and `lib/include/`.

## Key Design Decisions

1. **Compute shaders, not fragment shaders** — matches the engine's existing post-processing pattern (DenoiseFilter, RTAmbientOcclusion, TemporalAccumulator).
2. **Persistent pipeline** — unlike `GPUProcess::executeShader()` which creates/destroys per call, `SMAAProcessor` keeps pipelines alive, matching `DenoiseFilter`'s pattern.
3. **Per-frame resources** — uses `Engine::numImages()` to allocate one set of transient images per frame in flight.
4. **LUTs generated once** — `areaTexture` and `searchTexture` are generated on the first `process()` call using `GPUProcess` (immediate submit), then stored for the renderer lifetime.
5. **SMAA operates on the final composed image** — not per-layer, not before transparency. Input is the LDR image after tone mapping and color correction.
