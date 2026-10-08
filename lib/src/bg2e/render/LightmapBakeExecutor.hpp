#pragma once

#include <bg2e/base/Light.hpp>
#include <bg2e/render/LightmapBaker.hpp>
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/deferred/RTGlobalIllumination.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>

#include <vector>

namespace bg2e::render {

class LightmapCompositionPass;

class LightmapBakeExecutor final {
public:
    static void recordSample(
        LightmapBaker& baker,
        VkCommandBuffer commandBuffer,
        uint32_t frameNumber,
        uint32_t frameSlot,
        vulkan::FrameResources& frameResources,
        vulkan::DescriptorSetAllocator& descriptorAllocator,
        const vulkan::rt::RayTracingScene& rayTracingScene,
        deferred::RTAmbientOcclusion& rtaoPass,
        deferred::RTGlobalIllumination& rtgiPass,
        LightmapCompositionPass& compositionPass,
        vulkan::Image* irradianceMap,
        VkSampler irradianceSampler,
        const std::vector<base::LightData>& sceneLights);
};

}
