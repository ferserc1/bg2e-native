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

#include "LightmapCompositionPass.hpp"

#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

#include <stdexcept>

namespace bg2e::render {

LightmapCompositionPass::LightmapCompositionPass(Engine* engine)
    : _engine(engine)
{
    if (!_engine)
    {
        throw std::invalid_argument("LightmapCompositionPass: engine must not be null");
    }
    createPipeline();
}

LightmapCompositionPass::~LightmapCompositionPass()
{
    cleanup();
}

void LightmapCompositionPass::createPipeline()
{
    vulkan::factory::Sampler samplerFactory(_engine);
    samplerFactory.createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    _colorSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
    _maskSampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    vulkan::factory::DescriptorSetLayout descriptorLayoutFactory;
    descriptorLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // RTAO
    descriptorLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // normalized RTGI
    descriptorLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // valid mask
    descriptorLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);          // RGB factor output
    _descriptorSetLayout = descriptorLayoutFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_descriptorSetLayout);
    layoutFactory.addPushConstantRange(0, sizeof(PushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    _pipelineLayout = layoutFactory.build("LightmapCompositionPass::PipelineLayout");

    vulkan::factory::ComputePipeline pipelineFactory(_engine);
    pipelineFactory.setShader("lightmap_composition.comp.spv");
    _pipeline = pipelineFactory.build(_pipelineLayout, "LightmapCompositionPass::Pipeline");
}

void LightmapCompositionPass::render(
    VkCommandBuffer cmd,
    vulkan::DescriptorSetAllocator& descriptorAllocator,
    const GBufferManager& uvSurface,
    const vulkan::Image& aoImage,
    const vulkan::Image& giImage,
    vulkan::Image& output,
    LightmapMode mode)
{
    if (_pipeline == VK_NULL_HANDLE)
    {
        throw std::logic_error("LightmapCompositionPass::render called before pipeline creation");
    }
    if (uvSurface.imageCount() < 4 || uvSurface.depthImage())
    {
        throw std::invalid_argument("LightmapCompositionPass requires the depthless UV surface profile");
    }
    const VkExtent2D extent = output.extent2D();
    if (uvSurface.extent().width != extent.width || uvSurface.extent().height != extent.height ||
        aoImage.extent2D().width != extent.width || aoImage.extent2D().height != extent.height ||
        giImage.extent2D().width != extent.width || giImage.extent2D().height != extent.height ||
        output.format() != VK_FORMAT_R16G16B16A16_SFLOAT)
    {
        throw std::invalid_argument("LightmapCompositionPass requires native-resolution bake layers");
    }
    if (mode != LightmapMode::RTAO && mode != LightmapMode::RTGI)
    {
        throw std::invalid_argument("LightmapCompositionPass received an unsupported mode");
    }

    vulkan::Image::cmdTransitionImage(cmd, output.handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    std::unique_ptr<vulkan::DescriptorSet> descriptorSet(descriptorAllocator.allocate(_descriptorSetLayout));
    descriptorSet->beginUpdate();
    descriptorSet->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        &aoImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _colorSampler);
    descriptorSet->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        &giImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _colorSampler);
    descriptorSet->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(3).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _maskSampler);
    descriptorSet->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        &output, VK_IMAGE_LAYOUT_GENERAL);
    descriptorSet->endUpdate();

    const VkDescriptorSet descriptorSetHandle = descriptorSet->descriptorSet();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &descriptorSetHandle, 0, nullptr);

    const PushConstants pushConstants{ mode == LightmapMode::RTGI ? 1u : 0u };
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(PushConstants), &pushConstants);
    vkCmdDispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);

    vulkan::Image::cmdTransitionImage(cmd, output.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void LightmapCompositionPass::cleanup()
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
    if (_descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _descriptorSetLayout, nullptr);
        _descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (_colorSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_engine->device().handle(), _colorSampler, nullptr);
        _colorSampler = VK_NULL_HANDLE;
    }
    if (_maskSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(_engine->device().handle(), _maskSampler, nullptr);
        _maskSampler = VK_NULL_HANDLE;
    }
}

}
