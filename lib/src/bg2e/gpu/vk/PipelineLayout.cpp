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

PipelineLayout::PipelineLayout(VkDevice device, const gpu::PipelineLayoutDescription& description)
    : _device(device)
{
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
    if (_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
}

}
}
}
