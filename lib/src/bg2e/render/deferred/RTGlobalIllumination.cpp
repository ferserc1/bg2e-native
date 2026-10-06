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

#include <bg2e/render/deferred/RTGlobalIllumination.hpp>
#include <bg2e/render/BlueNoise.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <glm/glm.hpp>
#include <cstring>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bg2e::render::deferred {

RTGlobalIllumination::RTGlobalIllumination(Engine* engine)
    : _engine{engine}
{
}

RTGlobalIllumination::~RTGlobalIllumination()
{
    cleanup();
}

void RTGlobalIllumination::build(const GBufferManager* /*gbuffer*/, VkExtent2D extent)
{
    _extent = extent;

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    if (!_engine->rayTracingSupported())
    {
        createFallbackImage();
        _rtSupported = false;
        return;
    }

    _rtSupported = true;
    createGIResources(extent);
    createPipeline();
}

void RTGlobalIllumination::createFallbackImage()
{
    const uint32_t bpp = 4;
    size_t dataSize = 4 * 4 * bpp * sizeof(uint16_t);
    std::vector<uint8_t> blackData(dataSize, 0);

    _fallbackImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine, "RTGI: fallback image", blackData.data(),
            VkExtent2D{4, 4}, bpp, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        )
    );
}

void RTGlobalIllumination::createGIResources(VkExtent2D extent)
{
    cleanupImages();

    float scale = rtgiResolutionScale(_settings.quality);
    VkExtent2D scaledExtent = {
        static_cast<uint32_t>(std::round(extent.width * scale)),
        static_cast<uint32_t>(std::round(extent.height * scale))
    };

    _giImages.resize(_engine->numImages());
    for (uint32_t i = 0; i < _giImages.size(); ++i)
    {
        _giImages[i] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "RTGI output " + std::to_string(i),
                VK_FORMAT_R16G16B16A16_SFLOAT,
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

void RTGlobalIllumination::createPipeline()
{
    // Set 0: TLAS, output image, depth, normals, irradiance cubemap
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);   // blue noise
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR |
        VK_SHADER_STAGE_MISS_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    if (_materialDataBinding)
    {
        layoutFactory.addDescriptorSetLayout(_materialDataBinding->createLayout());
    }
    if (_reflectionLightDataBinding)
    {
        layoutFactory.addDescriptorSetLayout(
            _reflectionLightDataBinding->createLayout(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        );
    }
    layoutFactory.addPushConstantRange(
        0,
        sizeof(GIPushConstants),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    );
    _pipelineLayout = layoutFactory.build("RTGlobalIllumination::PipelineLayout");

    vulkan::factory::RayTracingPipeline rtPipelineFactory(_engine);
    rtPipelineFactory.setRayGenShader("rt_gi.rgen.spv");
    rtPipelineFactory.setMissShader("rt_gi.rmiss.spv");
    rtPipelineFactory.setClosestHitShader("rt_gi.rchit.spv");
    rtPipelineFactory.setAnyHitShader("rt_alpha_test.rahit.spv");

    _pipeline = rtPipelineFactory.build(_pipelineLayout, 1, "RTGlobalIllumination::Pipeline");
    auto sbt = rtPipelineFactory.createSBT("RTGlobalIllumination::SBT");

    _sbtBuffer = std::move(sbt.buffer);
    _raygenRegion = sbt.raygenRegion;
    _missRegion = sbt.missRegion;
    _hitRegion = sbt.hitRegion;
    _callableRegion = sbt.callableRegion;
}

void RTGlobalIllumination::buildUv()
{
    if (!_engine->rayTracingSupported())
    {
        throw std::runtime_error("RTGlobalIllumination::buildUv requires ray tracing support");
    }
    _rtSupported = true;
    createUvPipeline();
}

void RTGlobalIllumination::createUvPipeline()
{
    if (_uvPipeline != VK_NULL_HANDLE)
    {
        return;
    }
    if (!_materialDataBinding || !_reflectionLightDataBinding)
    {
        throw std::logic_error("RTGlobalIllumination::buildUv requires material and light bindings");
    }

    vulkan::factory::Sampler samplerFactory(_engine);
    _uvSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerFactory.createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    _uvMaskSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    vulkan::factory::DescriptorSetLayout descriptorLayoutFactory;
    descriptorLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    descriptorLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    descriptorLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _uvDsLayout = descriptorLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_uvDsLayout);
    layoutFactory.addDescriptorSetLayout(_materialDataBinding->createLayout(
        VK_SHADER_STAGE_RAYGEN_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR
    ));
    layoutFactory.addDescriptorSetLayout(
        _reflectionLightDataBinding->createLayout(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
    layoutFactory.addPushConstantRange(
        0, sizeof(GIPushConstants), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    _uvPipelineLayout = layoutFactory.build("RTGlobalIllumination::UvPipelineLayout");

    vulkan::factory::RayTracingPipeline pipelineFactory(_engine);
    pipelineFactory.setRayGenShader("rt_gi_uv.rgen.spv");
    pipelineFactory.setMissShader("rt_gi.rmiss.spv");
    pipelineFactory.setClosestHitShader("rt_gi.rchit.spv");
    pipelineFactory.setAnyHitShader("rt_alpha_test.rahit.spv");
    _uvPipeline = pipelineFactory.build(_uvPipelineLayout, 1, "RTGlobalIllumination::UvPipeline");
    auto sbt = pipelineFactory.createSBT("RTGlobalIllumination::UvSBT");
    _uvSbtBuffer = std::move(sbt.buffer);
    _uvRaygenRegion = sbt.raygenRegion;
    _uvMissRegion = sbt.missRegion;
    _uvHitRegion = sbt.hitRegion;
    _uvCallableRegion = sbt.callableRegion;
}

void RTGlobalIllumination::renderUv(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    vulkan::DescriptorSetAllocator& descriptorAllocator,
    const GBufferManager& uvSurface,
    const vulkan::rt::RayTracingScene& rayTracingScene,
    vulkan::Image& giOutput,
    vulkan::Image* irradianceMap,
    VkSampler irradianceSampler,
    const std::vector<base::LightData>& giLights,
    const LightmapSettings& settings)
{
    if (_uvPipeline == VK_NULL_HANDLE || !_rtSupported)
    {
        throw std::logic_error("RTGlobalIllumination::renderUv called before buildUv");
    }
    if (uvSurface.imageCount() < 4 || uvSurface.depthImage())
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv requires the four-attachment depthless UV surface profile");
    }
    if (uvSurface.image(0)->format() != VK_FORMAT_R32G32B32A32_SFLOAT ||
        uvSurface.image(1)->format() != VK_FORMAT_R16G16B16A16_SFLOAT ||
        uvSurface.image(3)->format() != VK_FORMAT_R32_UINT ||
        giOutput.format() != VK_FORMAT_R16G16B16A16_SFLOAT)
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv received incompatible surface or output formats");
    }
    if (uvSurface.extent().width != giOutput.extent2D().width ||
        uvSurface.extent().height != giOutput.extent2D().height)
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv requires matching UV surface and GI extents");
    }
    if (rayTracingScene.tlas() == VK_NULL_HANDLE || rayTracingScene.objectInstances().empty())
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv requires a non-empty context-owned TLAS");
    }
    if (!irradianceMap || irradianceSampler == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv requires the active renderer's irradiance map and sampler");
    }
    if (settings.samplesPerPixel == 0 || settings.giBounces == 0 ||
        settings.samplesPerPixel > static_cast<uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv requires positive samples and GI bounces");
    }
    if (!std::isfinite(settings.maxRayDistance) || settings.maxRayDistance <= 0.0f)
    {
        throw std::invalid_argument("RTGlobalIllumination::renderUv maxRayDistance must be finite and positive");
    }

    vulkan::Image::cmdTransitionImage(cmd, giOutput.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    std::unique_ptr<vulkan::DescriptorSet> inputSet(descriptorAllocator.allocate(_uvDsLayout));
    inputSet->beginUpdate();
    inputSet->addAccelerationStructure(0, rayTracingScene.tlas());
    inputSet->addImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &giOutput, VK_IMAGE_LAYOUT_GENERAL);
    inputSet->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(0).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _uvSampler);
    inputSet->addImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _uvSampler);
    inputSet->addImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        irradianceMap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, irradianceSampler);
    inputSet->addImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(3).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _uvMaskSampler);
    inputSet->endUpdate();

    const VkDescriptorSet inputSetHandle = inputSet->descriptorSet();
    const VkDescriptorSet materialSetHandle = _materialDataBinding->newDescriptorSet(
        frameResources, descriptorAllocator, rayTracingScene.objectInstances());
    const VkDescriptorSet lightSetHandle = _reflectionLightDataBinding->newDescriptorSet(
        frameResources, descriptorAllocator, giLights);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _uvPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _uvPipelineLayout, 0, 1, &inputSetHandle, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _uvPipelineLayout, 1, 1, &materialSetHandle, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _uvPipelineLayout, 2, 1, &lightSetHandle, 0, nullptr);

    const VkExtent2D extent = giOutput.extent2D();
    GIPushConstants pushConstants{};
    pushConstants.inverseViewProjection = glm::mat4(1.0f);
    pushConstants.cameraPosition = glm::vec3(0.0f);
    pushConstants.rayBias = 0.0017f;
    pushConstants.outputSize = glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height));
    pushConstants.sampleCount = settings.samplesPerPixel;
    pushConstants.bounceCount = settings.giBounces;
    pushConstants.frameIndex = currentFrame;
    pushConstants.maxDistance = settings.maxRayDistance;
    pushConstants.giLightCount = static_cast<uint32_t>(giLights.size());
    pushConstants.shadowSamples = 32;
    pushConstants.useBlueNoise = 0;
    pushConstants.useShadows = settings.rtShadows ? 1u : 0u;
    vkCmdPushConstants(cmd, _uvPipelineLayout,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        0, sizeof(GIPushConstants), &pushConstants);

    vulkan::cmdTraceRays(cmd,
        &_uvRaygenRegion, &_uvMissRegion, &_uvHitRegion, &_uvCallableRegion,
        extent.width, extent.height, 1);

    vulkan::Image::cmdTransitionImage(cmd, giOutput.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTGlobalIllumination::clearUv(VkCommandBuffer cmd, vulkan::Image& giOutput)
{
    vulkan::Image::cmdTransitionImage(cmd, giOutput.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    VkClearColorValue clearBlack{{0.0f, 0.0f, 0.0f, 0.0f}};
    const VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, giOutput.handle(), VK_IMAGE_LAYOUT_GENERAL, &clearBlack, 1, &range);
    vulkan::Image::cmdTransitionImage(cmd, giOutput.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTGlobalIllumination::cleanupUv()
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
    if (_uvSbtBuffer)
    {
        _uvSbtBuffer->cleanup();
        _uvSbtBuffer.reset();
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

void RTGlobalIllumination::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const GBufferManager* gbuffer,
    const glm::mat4& inverseViewProjection,
    const glm::vec3& cameraPosition,
    VkAccelerationStructureKHR tlas,
    const std::vector<vulkan::rt::RTObjectInstance>& objectInstances,
    vulkan::Image* irradianceMap,
    VkSampler irradianceSampler,
    const std::vector<base::LightData>& giLights
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
    auto output = _giImages.empty() ? _fallbackImage : _giImages[frameIndex];

    if (tlas == VK_NULL_HANDLE)
    {
        vulkan::Image::cmdTransitionImage(cmd, output->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clearBlack{{0.0f, 0.0f, 0.0f, 0.0f}};
        VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(cmd, output->handle(), VK_IMAGE_LAYOUT_GENERAL, &clearBlack, 1, &range);

        vulkan::Image::cmdTransitionImage(cmd, output->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return;
    }

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
        irradianceMap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, irradianceSampler);
    if (_blueNoise)
    {
        ds->addImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _blueNoise->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _blueNoise->sampler());
    }
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    if (_materialDataBinding && objectInstances.size() > 0)
    {
        auto materialDS = _materialDataBinding->newDescriptorSet(frameResources, objectInstances);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            _pipelineLayout, 1, 1, &materialDS, 0, nullptr);
    }

    if (_reflectionLightDataBinding)
    {
        auto lightDS = _reflectionLightDataBinding->newDescriptorSet(frameResources, giLights);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            _pipelineLayout, 2, 1, &lightDS, 0, nullptr);
    }

    float scale = rtgiResolutionScale(_settings.quality);
    VkExtent2D scaledExtent = {
        static_cast<uint32_t>(std::round(_extent.width * scale)),
        static_cast<uint32_t>(std::round(_extent.height * scale))
    };

    GIPushConstants pc{};
    pc.inverseViewProjection = inverseViewProjection;
    pc.cameraPosition = cameraPosition;
    pc.rayBias = _settings.rayBias;
    pc.outputSize = glm::vec2(static_cast<float>(scaledExtent.width), static_cast<float>(scaledExtent.height));
    pc.sampleCount = _settings.sampleCount;
    pc.bounceCount = _settings.bounceCount;
    pc.frameIndex = currentFrame;
    pc.maxDistance = _settings.maxDistance;
    pc.giLightCount = static_cast<uint32_t>(giLights.size());
    pc.shadowSamples = _settings.shadowSamples;
    pc.useBlueNoise = (_settings.useBlueNoise && _blueNoise) ? 1u : 0u;
    pc.useShadows = 1u;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        0, sizeof(GIPushConstants), &pc);

    vulkan::cmdTraceRays(
        cmd,
        &_raygenRegion,
        &_missRegion,
        &_hitRegion,
        &_callableRegion,
        scaledExtent.width,
        scaledExtent.height,
        1
    );

    vulkan::Image::cmdTransitionImage(cmd, output->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTGlobalIllumination::cleanup()
{
    cleanupUv();
    cleanupImages();

    if (_sbtBuffer)
    {
        _sbtBuffer->cleanup();
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

std::shared_ptr<vulkan::Image> RTGlobalIllumination::giImage(uint32_t frameIndex) const
{
    if (_giImages.empty() || frameIndex >= _giImages.size())
    {
        return _fallbackImage;
    }
    return _giImages[frameIndex];
}

void RTGlobalIllumination::setQuality(RTGIQuality quality)
{
    if (quality == _settings.quality)
    {
        return;
    }

    _engine->device().waitIdle();
    _settings.quality = quality;

    if (_rtSupported && !_giImages.empty())
    {
        createGIResources(_extent);
    }
}

void RTGlobalIllumination::resize(VkExtent2D extent)
{
    if (!_rtSupported)
    {
        return;
    }

    _extent = extent;
    createGIResources(extent);
}

void RTGlobalIllumination::cleanupImages()
{
    for (auto& img : _giImages)
    {
        if (img) img->cleanup();
    }
    _giImages.clear();

    if (_fallbackImage)
    {
        _fallbackImage->cleanup();
        _fallbackImage.reset();
    }
}

}
