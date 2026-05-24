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

#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <cmath>

namespace bg2e::render::deferred {

RTAmbientOcclusion::RTAmbientOcclusion(Engine * engine)
    : _engine{engine}
{
}

RTAmbientOcclusion::~RTAmbientOcclusion()
{
    cleanup();
}

void RTAmbientOcclusion::build(VkExtent2D extent)
{
    _extent = extent;

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build();

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    if (!_engine->rayTracingSupported())
    {
        createWhiteFallback();
        _rtSupported = false;
        return;
    }

    _rtSupported = true;
    createAOResources(extent);
    createPipeline();
}

void RTAmbientOcclusion::createWhiteFallback()
{
    uint8_t whiteData[16];
    memset(whiteData, 0xFF, sizeof(whiteData));

    auto img = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine, "RT AO fallback", whiteData,
            VkExtent2D{4, 4}, 1, VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        )
    );

    _aoImages.resize(_engine->numImages());
    for (auto& ao : _aoImages)
    {
        ao = img;
    }
}

void RTAmbientOcclusion::createAOResources(VkExtent2D extent)
{
    cleanupImages();

    _aoImages.resize(_engine->numImages());
    for (uint32_t i = 0; i < _aoImages.size(); i++)
    {
        _aoImages[i] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "RT AO image " + std::to_string(i),
                VK_FORMAT_R8_UNORM,
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

void RTAmbientOcclusion::createPipeline()
{
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(AOPushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT
    );
    _pipelineLayout = layoutFactory.build();

    vulkan::factory::ComputePipeline plFactory(_engine);
    plFactory.setShader("rt_ao.comp.spv");
    _pipeline = plFactory.build(_pipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}

void RTAmbientOcclusion::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources & frameResources,
    const GBufferManager * gbuffer,
    const glm::mat4 & inverseViewProjection
)
{
    if (!_rtSupported)
    {
        return;
    }

    VkAccelerationStructureKHR tlas = frameResources.rayTracingScene->tlas();
    auto aoImage = _aoImages[_engine->currentFrameResourcesIndex()];

    if (tlas == VK_NULL_HANDLE)
    {
        vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clearWhite{{1.0f, 0.0f, 0.0f, 0.0f}};
        VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, aoImage->handle(), VK_IMAGE_LAYOUT_GENERAL, &clearWhite, 1, &range);

        vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return;
    }

    vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    auto ds = frameResources.newDescriptorSet(_dsLayout);
    ds->beginUpdate();
    ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addAccelerationStructure(2, tlas);
    ds->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        aoImage.get(), VK_IMAGE_LAYOUT_GENERAL);
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    AOPushConstants pc{};
    pc.inverseViewProjection = inverseViewProjection;
    pc.sampleCount = 4;
    pc.bounceCount = 3;
    pc.radius = 0.6f;
    pc.bias = 0.01f;
    pc.falloff = 1.0f;
    pc.bounceAttenuation = 0.5f;
    pc.frameIndex = currentFrame;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(AOPushConstants), &pc);

    uint32_t groupX = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
    uint32_t groupY = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
    vkCmdDispatch(cmd, groupX, groupY, 1);

    vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTAmbientOcclusion::resize(VkExtent2D newExtent)
{
    if (!_rtSupported)
    {
        return;
    }
    _extent = newExtent;
    createAOResources(newExtent);
}

void RTAmbientOcclusion::cleanupImages()
{
    if (!_rtSupported)
    {
        if (!_aoImages.empty() && _aoImages[0])
        {
            _aoImages[0]->cleanup();
        }
    }
    else
    {
        for (auto& img : _aoImages)
        {
            if (img) img->cleanup();
        }
    }
    _aoImages.clear();
}

void RTAmbientOcclusion::cleanup()
{
    cleanupImages();
}

std::shared_ptr<vulkan::Image> RTAmbientOcclusion::aoImage(uint32_t frameIndex) const
{
    return _aoImages[frameIndex];
}

VkSampler RTAmbientOcclusion::sampler() const
{
    return _sampler;
}

bool RTAmbientOcclusion::rtSupported() const
{
    return _rtSupported;
}

}