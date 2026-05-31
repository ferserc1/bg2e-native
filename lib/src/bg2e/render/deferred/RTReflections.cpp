/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/render/deferred/RTReflections.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <glm/glm.hpp>
#include <cstring>

namespace bg2e::render::deferred {

RTReflections::RTReflections(Engine* engine)
    : _engine{engine}
{
}

RTReflections::~RTReflections()
{
    cleanup();
}

void RTReflections::build(const GBufferManager* gbuffer, VkExtent2D extent)
{
    _extent = extent;

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    createFallbackImage();

    if (!_engine->rayTracingSupported())
    {
        _rtSupported = false;
        return;
    }

    _rtSupported = true;
    createReflectionResources(extent);
    createPipeline();
}

void RTReflections::createFallbackImage()
{
    const uint32_t bpp = 4;
    size_t dataSize = 4 * 4 * bpp * sizeof(uint16_t);
    std::vector<uint8_t> whiteData(dataSize, 0);

    _fallbackImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine, "RT Reflections fallback", whiteData.data(),
            VkExtent2D{4, 4}, bpp, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        )
    );
}

void RTReflections::createReflectionResources(VkExtent2D extent)
{
    cleanupImages();

    _reflectionImages.resize(_engine->numImages());

    for (uint32_t i = 0; i < _reflectionImages.size(); ++i)
    {
        _reflectionImages[i] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "RT Reflections output " + std::to_string(i),
                VK_FORMAT_R16G16B16A16_SFLOAT,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );
    }
}

void RTReflections::createPipeline()
{
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR |
        VK_SHADER_STAGE_MISS_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(ReflectionPushConstants),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR
    );
    _pipelineLayout = layoutFactory.build("RTReflections::PipelineLayout");

    vulkan::factory::RayTracingPipeline rtPipelineFactory(_engine);
    rtPipelineFactory.setRayGenShader("rt_reflections.rgen.spv");
    rtPipelineFactory.setMissShader("rt_reflections.rmiss.spv");
    rtPipelineFactory.setClosestHitShader("rt_reflections.rchit.spv");

    _pipeline = rtPipelineFactory.build(_pipelineLayout, 1, "RTReflections::Pipeline");
    auto sbt = rtPipelineFactory.createSBT("RTReflections::SBT");

    _sbtBuffer = std::move(sbt.buffer);
    _raygenRegion = sbt.raygenRegion;
    _missRegion = sbt.missRegion;
    _hitRegion = sbt.hitRegion;
    _callableRegion = sbt.callableRegion;

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}

void RTReflections::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const GBufferManager* gbuffer,
    const glm::mat4& inverseViewProjection,
    const glm::vec3& cameraPosition,
    VkAccelerationStructureKHR tlas
)
{
    if (!_settings.enabled)
    {
        return;
    }

    if (!_pipeline)
    {
        return;
    }

    uint32_t frameIndex = _engine->currentFrameResourcesIndex();
    if (tlas == VK_NULL_HANDLE)
    {
        auto output = _reflectionImages.empty() ? _fallbackImage : _reflectionImages[frameIndex];

        vulkan::Image::cmdTransitionImage(cmd, output->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clearBlack{{0.0f, 0.0f, 0.0f, 0.0f}};
        VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, output->handle(), VK_IMAGE_LAYOUT_GENERAL, &clearBlack, 1, &range);

        vulkan::Image::cmdTransitionImage(cmd, output->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return;
    }

    auto output = _reflectionImages[frameIndex];
    vulkan::Image::cmdTransitionImage(cmd, output->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    auto ds = frameResources.newDescriptorSet(_dsLayout);
    ds->beginUpdate();
    ds->addAccelerationStructure(0, tlas);
    ds->addImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        output.get(), VK_IMAGE_LAYOUT_GENERAL);
    ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(2).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    ReflectionPushConstants pc{};
    pc.inverseViewProjection = inverseViewProjection;
    pc.cameraPosition = cameraPosition;
    pc.maxRoughness = _settings.maxRoughness;
    pc.outputSize = glm::vec2(static_cast<float>(_extent.width), static_cast<float>(_extent.height));
    pc.sampleCount = _settings.sampleCount;
    pc.frameIndex = currentFrame;
    pc.rayBias = _settings.rayBias;
    pc.maxDistance = _settings.maxDistance;
    pc.roughnessSpread = _settings.roughnessSpread;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(ReflectionPushConstants), &pc);

    vulkan::cmdTraceRays(
        cmd,
        &_raygenRegion,
        &_missRegion,
        &_hitRegion,
        &_callableRegion,
        _extent.width,
        _extent.height,
        1
    );

    vulkan::Image::cmdTransitionImage(cmd, output->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTReflections::cleanup()
{
    cleanupImages();

    if (_sbtBuffer)
    {
        _sbtBuffer.reset();
    }

    if (_pipeline)
    {
        vkDestroyPipeline(_engine->device().handle(), _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(_engine->device().handle(), _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    }

    if (_sampler)
    {
        vkDestroySampler(_engine->device().handle(), _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
}

std::shared_ptr<vulkan::Image> RTReflections::reflectionImage(uint32_t frameIndex) const
{
    if (_reflectionImages.empty() || frameIndex >= _reflectionImages.size())
    {
        return _fallbackImage;
    }
    return _reflectionImages[frameIndex];
}

void RTReflections::resize(VkExtent2D extent)
{
    if (!_rtSupported)
    {
        return;
    }

    _extent = extent;
    createReflectionResources(extent);
}

void RTReflections::cleanupImages()
{
    for(auto& img : _reflectionImages)
    {
        if (img) img->cleanup();
    }
    _reflectionImages.clear();

    if (_fallbackImage)
    {
        _fallbackImage->cleanup();
        _fallbackImage.reset();
    }
}

}