#include "LightmapBakeExecutor.hpp"

#include "LightmapCompositionPass.hpp"

#include <bg2e/scene/Node.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/BakerContext.hpp>
#include "UvSurfacePass.hpp"

#include <stdexcept>

namespace bg2e::render {

void LightmapBakeExecutor::recordSample(
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
    const std::vector<base::LightData>& sceneLights)
{
    if (baker._completedFrames >= baker._settings.accumulationFrames)
    {
        throw std::logic_error("LightmapBaker::update: accumulation is complete; reset before another update");
    }
    if (!baker._targetNode || baker._targetNode->sceneRoot() != baker._context->rootNode())
    {
        throw std::invalid_argument("LightmapBaker::update: target no longer belongs to the context root");
    }
    if (baker._settings.mode == LightmapMode::RTGI &&
        (!irradianceMap || irradianceSampler == VK_NULL_HANDLE))
    {
        throw std::logic_error("LightmapBaker::update: RTGI requires a usable irradiance environment");
    }

    std::vector<base::LightData> giLights;
    giLights.reserve(sceneLights.size());
    for (const auto& light : sceneLights)
    {
        if (light.affectsReflections != 0)
        {
            giLights.push_back(light);
        }
    }

    baker.recordUvSurface(commandBuffer, frameSlot);
    if (baker._settings.mode == LightmapMode::RTAO)
    {
        rtaoPass.renderUv(
            commandBuffer, frameNumber, frameResources, descriptorAllocator,
            baker._uvSurfacePass->manager(frameSlot), rayTracingScene,
            baker.aoImage(frameSlot), baker._settings.samplesPerPixel, 0.1f);
        rtgiPass.clearUv(commandBuffer, baker.giImage(frameSlot));
    }
    else
    {
        rtaoPass.clearUvNeutral(commandBuffer, baker.aoImage(frameSlot));
        rtgiPass.renderUv(
            commandBuffer, frameNumber, frameResources, descriptorAllocator,
            baker._uvSurfacePass->manager(frameSlot), rayTracingScene,
            baker.giImage(frameSlot), irradianceMap, irradianceSampler,
            giLights, baker._settings);
    }

    compositionPass.render(
        commandBuffer, descriptorAllocator,
        baker._uvSurfacePass->manager(frameSlot),
        baker.aoImage(frameSlot), baker.giImage(frameSlot),
        baker.resultImage(frameSlot), baker._settings.mode);
    baker.recordAccumulation(commandBuffer, descriptorAllocator, frameSlot);
}

}
