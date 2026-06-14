# Step 2: Class Header — `SMAAProcessor.hpp`

## File

`lib/include/bg2e/render/deferred/SMAAProcessor.hpp` (create)

## Purpose

Define the `bg2e::render::deferred::SMAAProcessor` class that encapsulates the three-pass SMAA compute pipeline and its resources.

## Namespace

```cpp
namespace bg2e {
namespace render {
namespace deferred {
```

## Class Definition

```cpp
class BG2E_API SMAAProcessor {
public:
    SMAAProcessor(Engine* engine);
    ~SMAAProcessor();

    // Create pipelines, LUT textures, and per-frame images.
    // Call once during RendererDeferred::build().
    // outputFormat should match the swapchain color image format.
    void build(VkExtent2D extent, VkFormat outputFormat = VK_FORMAT_R8G8B8A8_UNORM);

    // Recreate per-frame images at new viewport size.
    // Pipelines and LUT textures are unchanged.
    // Call during RendererDeferred::resize().
    void resize(VkExtent2D newExtent);

    // Execute the three SMAA passes on the given command buffer.
    // inputImage must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL on entry.
    // Returns a pointer to the per-frame outputImage (in SHADER_READ_ONLY_OPTIMAL layout).
    // The caller must transition inputImage back if needed after SMAA completes.
    const vulkan::Image* process(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        const vulkan::Image* inputImage
    );

    // Destroy all Vulkan resources.
    // Call during RendererDeferred::cleanup().
    void cleanup();

private:
    struct FrameData {
        std::unique_ptr<vulkan::Image> edgesImage;
        std::unique_ptr<vulkan::Image> blendWeightsImage;
        std::unique_ptr<vulkan::Image> outputImage;
    };

    Engine* _engine = nullptr;
    VkExtent2D _extent = {};
    VkFormat _outputFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // Samplers
    VkSampler _sampler = VK_NULL_HANDLE;

    // Persistent pipelines (created once, reused every frame)
    VkPipeline _edgeDetectionPipeline = VK_NULL_HANDLE;
    VkPipeline _blendWeightPipeline = VK_NULL_HANDLE;
    VkPipeline _neighborhoodBlendPipeline = VK_NULL_HANDLE;

    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout _edgeDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _blendWeightDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _neighborhoodBlendDSLayout = VK_NULL_HANDLE;

    // Static LUT textures
    std::unique_ptr<vulkan::Image> _areaTexture;
    std::unique_ptr<vulkan::Image> _searchTexture;
    bool _lutsGenerated = false;

    // Per-frame-in-flight resources
    std::vector<FrameData> _frames;

    // Internal helpers
    void createLUTTextures();
    void createPipelines();
    void createFrameImages(VkExtent2D extent);
    void cleanupFrameImages();

    void generateLUTs();

    // Push constant struct (shared across all three passes)
    struct SMAAPushConstants {
        glm::vec2 texelSize;  // 1.0 / viewportSize
    };
};
```

## Push Constants

All three SMAA passes share the same push constant struct:

```cpp
struct SMAAPushConstants {
    glm::vec2 texelSize;  // 1.0 / viewportSize (8 bytes, padded to 16)
};
```

This is passed to each compute dispatch. The `texelSize` value allows the shader to compute UV coordinates and sample neighbors correctly at any viewport resolution.

## Image Formats

| Image | Format | Usage Flags | Notes |
|-------|--------|-------------|-------|
| `edgesImage` | `VK_FORMAT_R8G8_UNORM` | `STORAGE_BIT \| SAMPLED_BIT` | Per-frame, viewport-sized |
| `blendWeightsImage` | `VK_FORMAT_R8G8B8A8_UNORM` | `STORAGE_BIT \| SAMPLED_BIT` | Per-frame, viewport-sized |
| `outputImage` | `VK_FORMAT_R8G8B8A8_UNORM` | `STORAGE_BIT \| SAMPLED_BIT \| TRANSFER_SRC_BIT` | Per-frame, viewport-sized |
| `_areaTexture` | `VK_FORMAT_R8G8_UNORM` | `STORAGE_BIT \| SAMPLED_BIT` | 256×256, static |
| `_searchTexture` | `VK_FORMAT_R8_UNORM` | `STORAGE_BIT \| SAMPLED_BIT` | 64×64, static |

The `outputImage` format matches the typical swapchain image format (`R8G8B8A8_UNORM`). If the engine uses a different format (e.g., `B8G8R8A8_UNORM`), the output image format should match `colorImageFormat` passed to the renderer. Consider making this configurable via `build()`.

## Descriptor Set Layouts

### Edge Detection (`_edgeDSLayout`)

| Binding | Type | Description |
|---------|------|-------------|
| 0 | `COMBINED_IMAGE_SAMPLER` | `inputImage` (final composed image) |
| 1 | `STORAGE_IMAGE` | `edgesOutput` |

### Blend Weight (`_blendWeightDSLayout`)

| Binding | Type | Description |
|---------|------|-------------|
| 0 | `COMBINED_IMAGE_SAMPLER` | `edgesImage` |
| 1 | `COMBINED_IMAGE_SAMPLER` | `areaTexture` |
| 2 | `COMBINED_IMAGE_SAMPLER` | `searchTexture` |
| 3 | `STORAGE_IMAGE` | `blendWeightsOutput` |

### Neighborhood Blend (`_neighborhoodBlendDSLayout`)

| Binding | Type | Description |
|---------|------|-------------|
| 0 | `COMBINED_IMAGE_SAMPLER` | `inputImage` (final composed image) |
| 1 | `COMBINED_IMAGE_SAMPLER` | `blendWeightsImage` |
| 2 | `STORAGE_IMAGE` | `outputImage` |

## Pipeline Layout

All three pipelines share a single `VkPipelineLayout` (`_pipelineLayout`) with:
- One descriptor set layout (different per pipeline, but same layout handle count)
- One push constant range: `offset=0, size=sizeof(SMAAPushConstants), stageFlags=VK_SHADER_STAGE_COMPUTE_BIT`

## Includes Required

```cpp
#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <glm/glm.hpp>
#include <vector>
#include <memory>
```
