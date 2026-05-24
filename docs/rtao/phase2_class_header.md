# Phase 2: Class Header — `RTAmbientOcclusion.hpp`

## File

`lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp` (create)

## Namespace

```cpp
namespace bg2e::render::deferred
```

## Includes

```cpp
#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <vector>
#include <memory>
```

## Class Declaration

```cpp
class BG2E_API RTAmbientOcclusion {
public:
    RTAmbientOcclusion(Engine* engine);
    ~RTAmbientOcclusion();

    void build(VkExtent2D extent);
    void resize(VkExtent2D newExtent);
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        const GBufferManager* gbuffer,
        const glm::mat4& inverseViewProjection
    );
    void cleanup();

    std::shared_ptr<vulkan::Image> aoImage(uint32_t frameIndex) const;
    VkSampler sampler() const;
    bool rtSupported() const;

private:
    Engine* _engine;
    VkExtent2D _extent;
    bool _rtSupported = false;

    std::vector<std::shared_ptr<vulkan::Image>> _aoImages;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    struct AOPushConstants {
        glm::mat4 inverseViewProjection;
        int sampleCount;
        glm::vec3 padding;
    };

    void createWhiteFallback();
    void createAOResources(VkExtent2D extent);
    void createPipeline();
    void cleanupImages();
};
```

## Method Descriptions

| Method | Description |
|--------|-------------|
| `build(extent)` | Checks RT support, creates images (or 4x4 fallback), creates compute pipeline |
| `resize(newExtent)` | Recreates AO images at new size. No-op if `!_rtSupported` |
| `render(cmd, frame, frameResources, gbuffer, invVP)` | Dispatches compute shader. Clears to white if no TLAS. No-op if `!_rtSupported` |
| `cleanup()` | Destroys pipeline, layout, sampler, images |
| `aoImage(frameIndex)` | Returns the AO image for the given frame resource index |
| `sampler()` | Returns the sampler used for AO image sampling |
| `rtSupported()` | Returns whether RT is available |
| `createWhiteFallback()` | Creates 4x4 white R8_UNORM image shared across all frames |
| `createAOResources(extent)` | Creates one R8_UNORM image per frame resource |
| `createPipeline()` | Builds persistent compute pipeline with descriptor set layout |
| `cleanupImages()` | Helper to cleanup all AO images |

## Push Constants Struct

```cpp
struct AOPushConstants {
    glm::mat4 inverseViewProjection;  // 64 bytes
    int sampleCount;                   // 4 bytes
    glm::vec3 padding;                 // 12 bytes
};  // Total: 80 bytes
```

## Reference Files

- `lib/include/bg2e/render/gbuffer/GBufferManager.hpp` — similar per-frame image management pattern
- `lib/include/bg2e/render/vulkan/factory/ComputePipeline.hpp` — compute pipeline factory
- `lib/include/bg2e/render/vulkan/factory/DescriptorSetLayout.hpp` — descriptor set layout factory
- `lib/include/bg2e/render/vulkan/factory/PipelineLayout.hpp` — pipeline layout factory
- `lib/include/bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp` — TLAS binding pattern
