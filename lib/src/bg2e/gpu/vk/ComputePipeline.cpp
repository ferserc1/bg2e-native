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

#include <bg2e/gpu/vk/ComputePipeline.hpp>
#include <bg2e/gpu/vk/ShaderModule.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

ComputePipeline::ComputePipeline(gpu::Device* gpuDevice, VkDevice device, const gpu::ComputePipelineDescription& description)
    : gpu::ComputePipeline(gpuDevice), _device(device)
{
    auto* vkCsModule = dynamic_cast<vk::ShaderModule*>(description.computeShader);
    auto* vkLayout   = dynamic_cast<vk::PipelineLayout*>(description.layout);

    if (!vkCsModule || !vkLayout)
    {
        throw std::runtime_error("vk::ComputePipeline: description contains non-Vulkan objects");
    }

    VkPipelineShaderStageCreateInfo csStage{};
    csStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    csStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    csStage.module = vkCsModule->handle();
    csStage.pName = vkCsModule->entryPoint().c_str();

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = csStage;
    pipelineInfo.layout = vkLayout->handle();
    _layoutHandle = vkLayout->handle();

    VkResult result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
    if (result != VK_SUCCESS)
    {
        bg2e_log_debug << "vk::ComputePipeline: vkCreateComputePipelines failed with result " << result << bg2e_log_end;
        throw std::runtime_error("Failed to create Vulkan compute pipeline");
    }

    if (base::Log::isDebug() && !description.debugName.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(_pipeline);
        nameInfo.pObjectName = description.debugName.c_str();
        setDebugUtilsObjectName(_device, &nameInfo);
    }
}

ComputePipeline::~ComputePipeline()
{
    cleanup();
}

bool ComputePipeline::isValid() const
{
    return _pipeline != VK_NULL_HANDLE;
}

void ComputePipeline::cleanup()
{
    if (_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
}

}
}
}
