/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

VkShaderStageFlags shaderStageToVkFlags(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:  return VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return VK_SHADER_STAGE_VERTEX_BIT;
}

static VkDescriptorType toVkDescriptorType(ResourceType type)
{
    switch (type)
    {
        case ResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ResourceType::SampledImage:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ResourceType::StorageImage:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case ResourceType::Sampler:       return VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
}

PipelineLayout::PipelineLayout(VkDevice device, const gpu::PipelineLayoutDescription& description)
    : _device(device)
    , _description(description)
{
    // Group resource bindings by set index and create descriptor set layouts
    if (!description.resourceBindings.empty())
    {
        struct BindingWithSet
        {
            uint32_t set;
            VkDescriptorSetLayoutBinding binding;
        };

        std::vector<BindingWithSet> bindingsBySet;
        uint32_t maxSet = 0;

        for (const auto& rb : description.resourceBindings)
        {
            VkDescriptorSetLayoutBinding vkBinding{};
            vkBinding.binding = rb.binding;
            vkBinding.descriptorType = toVkDescriptorType(rb.type);
            vkBinding.descriptorCount = rb.count;
            vkBinding.stageFlags = shaderStageToVkFlags(rb.stage);

            bindingsBySet.push_back({ rb.set, vkBinding });
            if (rb.set > maxSet) maxSet = rb.set;
        }

        _descriptorSetLayouts.resize(maxSet + 1, VK_NULL_HANDLE);

        std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutsBySet(maxSet + 1);
        for (const auto& bws : bindingsBySet)
        {
            layoutsBySet[bws.set].push_back(bws.binding);
        }

        for (uint32_t i = 0; i < layoutsBySet.size(); ++i)
        {
            if (layoutsBySet[i].empty()) continue;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(layoutsBySet[i].size());
            layoutInfo.pBindings = layoutsBySet[i].data();

            VkResult result = vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayouts[i]);
            if (result != VK_SUCCESS)
            {
                bg2e_log_error << "vk::PipelineLayout: vkCreateDescriptorSetLayout failed with result " << result << bg2e_log_end;
                throw std::runtime_error("Failed to create Vulkan descriptor set layout");
            }
        }
    }

    // Push constant ranges
    std::vector<VkPushConstantRange> vkRanges;
    for (const auto& range : description.pushConstants)
    {
        VkPushConstantRange vkRange{};
        vkRange.offset = range.offset;
        vkRange.size   = range.size;
        vkRange.stageFlags = shaderStageToVkFlags(range.stage);
        vkRanges.push_back(vkRange);
    }

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = static_cast<uint32_t>(_descriptorSetLayouts.size());
    createInfo.pSetLayouts = _descriptorSetLayouts.data();
    createInfo.pushConstantRangeCount = static_cast<uint32_t>(vkRanges.size());
    createInfo.pPushConstantRanges = vkRanges.data();

    VkResult result = vkCreatePipelineLayout(_device, &createInfo, nullptr, &_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        bg2e_log_error << "vk::PipelineLayout: vkCreatePipelineLayout failed with result " << result << bg2e_log_end;
        throw std::runtime_error("Failed to create Vulkan pipeline layout");
    }
}

PipelineLayout::~PipelineLayout()
{
    cleanup();
}

bool PipelineLayout::isValid() const
{
    return _pipelineLayout != VK_NULL_HANDLE;
}

void PipelineLayout::cleanup()
{
    for (auto layout : _descriptorSetLayouts)
    {
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(_device, layout, nullptr);
        }
    }
    _descriptorSetLayouts.clear();

    if (_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
}

VkDescriptorSetLayout PipelineLayout::descriptorSetLayout(uint32_t set) const
{
    if (set >= _descriptorSetLayouts.size()) return VK_NULL_HANDLE;
    return _descriptorSetLayouts[set];
}

}
}
}
