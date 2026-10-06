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
#include <bg2e/render/BlueNoise.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

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

    float scale = rtaoResolutionScale(_quality);
    VkExtent2D scaledExtent = {
        static_cast<uint32_t>(std::round(extent.width * scale)),
        static_cast<uint32_t>(std::round(extent.height * scale))
    };

    _aoImages.resize(_engine->numImages());
    for (uint32_t i = 0; i < _aoImages.size(); i++)
    {
        _aoImages[i] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "RT AO image " + std::to_string(i),
                VK_FORMAT_R8_UNORM,
                scaledExtent,
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
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    if (_materialDataBinding)
    {
        layoutFactory.addDescriptorSetLayout(_materialDataBinding->createLayout());
    }
    layoutFactory.addPushConstantRange(
        0,
        sizeof(AOPushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT
    );
    _pipelineLayout = layoutFactory.build("RTAmbientOcclusion::PipelineLayout");

    vulkan::factory::ComputePipeline plFactory(_engine);
    plFactory.setShader("rt_ao.comp.spv");
    _pipeline = plFactory.build(_pipelineLayout, "RTAmbientOcclusion::Pipeline");

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}

void RTAmbientOcclusion::buildUv()
{
    if (!_engine->rayTracingSupported())
    {
        throw std::runtime_error("RTAmbientOcclusion::buildUv requires ray tracing support");
    }
    _rtSupported = true;
    createUvPipeline();
}

void RTAmbientOcclusion::createUvPipeline()
{
    if (_uvPipeline != VK_NULL_HANDLE)
    {
        return;
    }
    if (!_materialDataBinding)
    {
        throw std::logic_error("RTAmbientOcclusion::buildUv requires an RT material data binding");
    }

    vulkan::factory::Sampler samplerFactory(_engine);
    _uvSampler = samplerFactory.build();
    samplerFactory.createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    _uvMaskSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    vulkan::factory::DescriptorSetLayout descriptorLayoutFactory;
    descriptorLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    descriptorLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _uvDsLayout = descriptorLayoutFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_uvDsLayout);
    layoutFactory.addDescriptorSetLayout(_materialDataBinding->createLayout(VK_SHADER_STAGE_COMPUTE_BIT));
    layoutFactory.addPushConstantRange(
        0, sizeof(UvAOPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    _uvPipelineLayout = layoutFactory.build("RTAmbientOcclusion::UvPipelineLayout");

    vulkan::factory::ComputePipeline pipelineFactory(_engine);
    pipelineFactory.setShader("rt_ao_uv.comp.spv");
    _uvPipeline = pipelineFactory.build(_uvPipelineLayout, "RTAmbientOcclusion::UvPipeline");
}

void RTAmbientOcclusion::renderUv(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    vulkan::DescriptorSetAllocator& descriptorAllocator,
    const GBufferManager& uvSurface,
    const vulkan::rt::RayTracingScene& rayTracingScene,
    vulkan::Image& aoOutput,
    uint32_t samplesPerPixel,
    float maxRayDistance)
{
    if (!_rtSupported || _uvPipeline == VK_NULL_HANDLE || _uvPipelineLayout == VK_NULL_HANDLE)
    {
        throw std::logic_error("RTAmbientOcclusion::renderUv called before buildUv");
    }
    if (uvSurface.imageCount() < 4 || uvSurface.depthImage())
    {
        throw std::invalid_argument("RTAmbientOcclusion::renderUv requires the four-attachment depthless UV surface profile");
    }
    if (uvSurface.image(0)->format() != VK_FORMAT_R32G32B32A32_SFLOAT ||
        uvSurface.image(1)->format() != VK_FORMAT_R16G16B16A16_SFLOAT ||
        uvSurface.image(3)->format() != VK_FORMAT_R32_UINT ||
        aoOutput.format() != VK_FORMAT_R8_UNORM)
    {
        throw std::invalid_argument("RTAmbientOcclusion::renderUv received incompatible surface or output formats");
    }
    if (uvSurface.extent().width != aoOutput.extent2D().width ||
        uvSurface.extent().height != aoOutput.extent2D().height)
    {
        throw std::invalid_argument("RTAmbientOcclusion::renderUv requires matching UV surface and AO extents");
    }
    if (samplesPerPixel == 0 || samplesPerPixel > static_cast<uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument("RTAmbientOcclusion::renderUv samplesPerPixel is out of range");
    }
    if (!std::isfinite(maxRayDistance) || maxRayDistance <= 0.0f)
    {
        throw std::invalid_argument("RTAmbientOcclusion::renderUv maxRayDistance must be finite and positive");
    }
    if (rayTracingScene.tlas() == VK_NULL_HANDLE || rayTracingScene.objectInstances().empty())
    {
        throw std::invalid_argument("RTAmbientOcclusion::renderUv requires a non-empty context-owned TLAS");
    }
    vulkan::Image::cmdTransitionImage(cmd, aoOutput.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    std::unique_ptr<vulkan::DescriptorSet> inputSet(descriptorAllocator.allocate(_uvDsLayout));
    inputSet->beginUpdate();
    inputSet->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(0).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _uvSampler);
    inputSet->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _uvSampler);
    inputSet->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(3).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _uvMaskSampler);
    inputSet->addAccelerationStructure(3, rayTracingScene.tlas());
    inputSet->addImage(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        &aoOutput, VK_IMAGE_LAYOUT_GENERAL);
    inputSet->endUpdate();

    const VkDescriptorSet inputSetHandle = inputSet->descriptorSet();
    const VkDescriptorSet materialSetHandle = _materialDataBinding->newDescriptorSet(
        frameResources, descriptorAllocator, rayTracingScene.objectInstances());

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _uvPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _uvPipelineLayout, 0, 1, &inputSetHandle, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _uvPipelineLayout, 1, 1, &materialSetHandle, 0, nullptr);

    UvAOPushConstants pushConstants{};
    pushConstants.sampleCount = static_cast<int>(samplesPerPixel);
    pushConstants.maxRayDistance = maxRayDistance;
    pushConstants.bias = _bias;
    pushConstants.falloff = _falloff;
    pushConstants.frameIndex = currentFrame;
    vkCmdPushConstants(cmd, _uvPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(UvAOPushConstants), &pushConstants);

    const VkExtent2D extent = aoOutput.extent2D();
    vkCmdDispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);

    vulkan::Image::cmdTransitionImage(cmd, aoOutput.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTAmbientOcclusion::cleanupUv()
{
    if (_uvPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _uvPipeline, nullptr);
        _uvPipeline = VK_NULL_HANDLE;
    }
    if (_uvPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _uvPipelineLayout, nullptr);
        _uvPipelineLayout = VK_NULL_HANDLE;
    }
    if (_uvDsLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _uvDsLayout, nullptr);
        _uvDsLayout = VK_NULL_HANDLE;
    }
    if (_uvSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_engine->device().handle(), _uvSampler, nullptr);
        _uvSampler = VK_NULL_HANDLE;
    }
    if (_uvMaskSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_engine->device().handle(), _uvMaskSampler, nullptr);
        _uvMaskSampler = VK_NULL_HANDLE;
    }
}

void RTAmbientOcclusion::clearUvNeutral(VkCommandBuffer cmd, vulkan::Image& aoOutput)
{
    vulkan::Image::cmdTransitionImage(cmd, aoOutput.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    VkClearColorValue clearWhite{{1.0f, 0.0f, 0.0f, 0.0f}};
    const VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, aoOutput.handle(), VK_IMAGE_LAYOUT_GENERAL, &clearWhite, 1, &range);
    vulkan::Image::cmdTransitionImage(cmd, aoOutput.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
    if (_blueNoise)
    {
        ds->addImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _blueNoise->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _blueNoise->sampler());
    }
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    if (_materialDataBinding)
    {
        const auto& objectInstances = frameResources.rayTracingScene->objectInstances();
        auto matDS = _materialDataBinding->newDescriptorSet(frameResources, objectInstances);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _pipelineLayout, 1, 1, &matDS, 0, nullptr);
    }

    AOPushConstants pc{};
    pc.inverseViewProjection = inverseViewProjection;
    pc.sampleCount = _sampleCount;
    pc.bounceCount = _bounceCount;
    pc.radius = _radius;
    pc.bias = _bias;
    pc.falloff = _falloff;
    pc.bounceAttenuation = _bounceAttenuation;
    pc.frameIndex = currentFrame;
    pc.useBlueNoise = (_useBlueNoise && _blueNoise) ? 1u : 0u;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(AOPushConstants), &pc);

    float scale = rtaoResolutionScale(_quality);
    uint32_t scaledWidth = static_cast<uint32_t>(std::round(_extent.width * scale));
    uint32_t scaledHeight = static_cast<uint32_t>(std::round(_extent.height * scale));
    uint32_t groupX = static_cast<uint32_t>(std::ceil(scaledWidth / 8.0f));
    uint32_t groupY = static_cast<uint32_t>(std::ceil(scaledHeight / 8.0f));
    vkCmdDispatch(cmd, groupX, groupY, 1);

    vulkan::Image::cmdTransitionImage(cmd, aoImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTAmbientOcclusion::setQuality(RTAOQuality quality)
{
    if (quality == _quality)
    {
        return;
    }

    _engine->device().waitIdle();

    _quality = quality;

    if (_rtSupported && !_aoImages.empty())
    {
        createAOResources(_extent);
    }
}

RTAOQuality RTAmbientOcclusion::quality() const
{
    return _quality;
}

void RTAmbientOcclusion::setAOSampleCount(int count)
{
    _sampleCount = count;
}

int RTAmbientOcclusion::aoSampleCount() const
{
    return _sampleCount;
}

void RTAmbientOcclusion::setAOBounceCount(int count)
{
    _bounceCount = count;
}

int RTAmbientOcclusion::aoBounceCount() const
{
    return _bounceCount;
}

void RTAmbientOcclusion::setAORadius(float radius)
{
    _radius = radius;
}

float RTAmbientOcclusion::aoRadius() const
{
    return _radius;
}

void RTAmbientOcclusion::setAOBias(float bias)
{
    _bias = bias;
}

float RTAmbientOcclusion::aoBias() const
{
    return _bias;
}

void RTAmbientOcclusion::setAOFalloff(float falloff)
{
    _falloff = falloff;
}

float RTAmbientOcclusion::aoFalloff() const
{
    return _falloff;
}

void RTAmbientOcclusion::setAOBounceAttenuation(float attenuation)
{
    _bounceAttenuation = attenuation;
}

float RTAmbientOcclusion::aoBounceAttenuation() const
{
    return _bounceAttenuation;
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
    cleanupUv();
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
