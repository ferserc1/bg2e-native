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

#include "RTShadowVisibilityPass.hpp"

#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>

#include <memory>
#include <stdexcept>

namespace bg2e::render::deferred {

RTShadowVisibilityPass::RTShadowVisibilityPass(
    Engine* engine,
    vulkan::rt::RTMaterialDataBinding* materialDataBinding,
    vulkan::rt::ReflectionLightDataBinding* lightDataBinding)
    : _engine(engine),
      _materialDataBinding(materialDataBinding),
      _lightDataBinding(lightDataBinding)
{
    if (!_engine || !_materialDataBinding || !_lightDataBinding)
    {
        throw std::invalid_argument("RTShadowVisibilityPass requires an engine and shared scene bindings");
    }
    createPipeline();
}

RTShadowVisibilityPass::~RTShadowVisibilityPass()
{
    cleanup();
}

void RTShadowVisibilityPass::createPipeline()
{
    vulkan::factory::Sampler samplerFactory(_engine);
    samplerFactory.createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    _surfaceSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
    _maskSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    vulkan::factory::DescriptorSetLayout surfaceLayoutFactory;
    surfaceLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    surfaceLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    surfaceLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    surfaceLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    surfaceLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _surfaceLayout = surfaceLayoutFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_surfaceLayout);
    layoutFactory.addDescriptorSetLayout(_materialDataBinding->createLayout(VK_SHADER_STAGE_COMPUTE_BIT));
    layoutFactory.addDescriptorSetLayout(_lightDataBinding->createLayout(VK_SHADER_STAGE_COMPUTE_BIT));
    layoutFactory.addPushConstantRange(0, sizeof(PushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    _pipelineLayout = layoutFactory.build("RTShadowVisibilityPass::PipelineLayout");

    vulkan::factory::ComputePipeline pipelineFactory(_engine);
    pipelineFactory.setShader("lightmap_shadow_uv.comp.spv");
    _pipeline = pipelineFactory.build(_pipelineLayout, "RTShadowVisibilityPass::Pipeline");
}

void RTShadowVisibilityPass::renderUv(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    vulkan::DescriptorSetAllocator& descriptorAllocator,
    const GBufferManager& uvSurface,
    const vulkan::rt::RayTracingScene& rayTracingScene,
    const std::vector<base::LightData>& lights,
    vulkan::Image& shadowOutput)
{
    if (_pipeline == VK_NULL_HANDLE)
    {
        throw std::logic_error("RTShadowVisibilityPass::renderUv called before pipeline creation");
    }
    if (uvSurface.imageCount() < 4 || uvSurface.depthImage())
    {
        throw std::invalid_argument("RTShadowVisibilityPass::renderUv requires the four-attachment depthless UV surface profile");
    }
    if (uvSurface.image(0)->format() != VK_FORMAT_R32G32B32A32_SFLOAT ||
        uvSurface.image(1)->format() != VK_FORMAT_R16G16B16A16_SFLOAT ||
        uvSurface.image(3)->format() != VK_FORMAT_R32_UINT ||
        shadowOutput.format() != VK_FORMAT_R8_UNORM ||
        uvSurface.extent().width != shadowOutput.extent2D().width ||
        uvSurface.extent().height != shadowOutput.extent2D().height)
    {
        throw std::invalid_argument("RTShadowVisibilityPass::renderUv received incompatible UV data or output");
    }
    if (rayTracingScene.tlas() == VK_NULL_HANDLE || rayTracingScene.objectInstances().empty())
    {
        throw std::invalid_argument("RTShadowVisibilityPass::renderUv requires the context-owned TLAS");
    }

    vulkan::Image::cmdTransitionImage(cmd, shadowOutput.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    std::unique_ptr<vulkan::DescriptorSet> surfaceSet(descriptorAllocator.allocate(_surfaceLayout));
    surfaceSet->beginUpdate();
    surfaceSet->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(0).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _surfaceSampler);
    surfaceSet->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _surfaceSampler);
    surfaceSet->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(3).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _maskSampler);
    surfaceSet->addAccelerationStructure(3, rayTracingScene.tlas());
    surfaceSet->addImage(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        &shadowOutput, VK_IMAGE_LAYOUT_GENERAL);
    surfaceSet->endUpdate();

    const VkDescriptorSet surfaceSetHandle = surfaceSet->descriptorSet();
    const VkDescriptorSet materialSetHandle = _materialDataBinding->newDescriptorSet(
        frameResources, descriptorAllocator, rayTracingScene.objectInstances());
    const VkDescriptorSet lightSetHandle = _lightDataBinding->newDescriptorSet(
        frameResources, descriptorAllocator, lights);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &surfaceSetHandle, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 1, 1, &materialSetHandle, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 2, 1, &lightSetHandle, 0, nullptr);

    const PushConstants pushConstants{ static_cast<uint32_t>(lights.size()) };
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(PushConstants), &pushConstants);

    const VkExtent2D extent = shadowOutput.extent2D();
    vkCmdDispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);

    vulkan::Image::cmdTransitionImage(cmd, shadowOutput.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    (void)currentFrame;
}

void RTShadowVisibilityPass::clearNeutral(VkCommandBuffer cmd, vulkan::Image& shadowOutput)
{
    vulkan::Image::cmdTransitionImage(cmd, shadowOutput.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    VkClearColorValue clearWhite{{1.0f, 0.0f, 0.0f, 0.0f}};
    const VkImageSubresourceRange range = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, shadowOutput.handle(), VK_IMAGE_LAYOUT_GENERAL, &clearWhite, 1, &range);
    vulkan::Image::cmdTransitionImage(cmd, shadowOutput.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void RTShadowVisibilityPass::cleanup()
{
    if (_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_surfaceLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _surfaceLayout, nullptr);
        _surfaceLayout = VK_NULL_HANDLE;
    }
    if (_surfaceSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_engine->device().handle(), _surfaceSampler, nullptr);
        _surfaceSampler = VK_NULL_HANDLE;
    }
    if (_maskSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_engine->device().handle(), _maskSampler, nullptr);
        _maskSampler = VK_NULL_HANDLE;
    }
}

}
