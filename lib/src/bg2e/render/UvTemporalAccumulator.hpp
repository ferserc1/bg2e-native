#pragma once

#include <bg2e/render/LightmapBaker.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <array>
#include <memory>

namespace bg2e::render {

class Engine;
class GBufferManager;
namespace vulkan { class DescriptorSetAllocator; }

// Progressive, same-texel averaging in atlas space. The two history images
// remain independent of frame slots; ordered graphics-queue submissions and
// image barriers preserve dependencies between consecutive bake frames.
class UvTemporalAccumulator {
public:
    UvTemporalAccumulator(Engine* engine, VkExtent2D extent);
    ~UvTemporalAccumulator();

    UvTemporalAccumulator(const UvTemporalAccumulator&) = delete;
    UvTemporalAccumulator& operator=(const UvTemporalAccumulator&) = delete;

    void record(VkCommandBuffer cmd,
                vulkan::DescriptorSetAllocator& descriptors,
                const vulkan::Image& sample,
                const GBufferManager& uvSurface,
                uint32_t previousSamples);
    void reset();
    [[nodiscard]] std::shared_ptr<vulkan::Image> image() const;
    [[nodiscard]] LightmapPixels readPixels(LightmapPixelFormat format, float exposureEV = 0.0f) const;

private:
    Engine* _engine;
    VkExtent2D _extent;
    std::array<std::shared_ptr<vulkan::Image>, 2> _history;
    uint32_t _active = 0;
    bool _hasResult = false;
    bool _initialized[2] = { false, false };
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
};

}
