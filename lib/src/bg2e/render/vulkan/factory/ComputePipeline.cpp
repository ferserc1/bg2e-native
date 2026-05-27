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

#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/ShaderModule.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/extensions.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

ComputePipeline::ComputePipeline(Engine * engine)
    :_engine(engine)
    , _shaderModule(VK_NULL_HANDLE)
{
}

ComputePipeline::~ComputePipeline()
{
    if (_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_engine->device().handle(), _shaderModule, nullptr);
    }
}

void ComputePipeline::setShader(const std::string& fileName, const std::string& entryPoint, const std::string& basePath)
{
    if (_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_engine->device().handle(), _shaderModule, nullptr);
        _shaderModule = VK_NULL_HANDLE;
    }
    auto shaderModule = ShaderModule::loadFromSPV(fileName, _engine->device().handle(), basePath);
    setShader(shaderModule, entryPoint);
}

void ComputePipeline::setShader(VkShaderModule shaderModule, const std::string& entryPoint)
{
    if (_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_engine->device().handle(), _shaderModule, nullptr);
    }
    _shaderModule = shaderModule;

    _shaderEntryPoint = entryPoint;
    _shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    _shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    _shaderStageInfo.module = _shaderModule;
    _shaderStageInfo.pName = _shaderEntryPoint.c_str();
}

VkPipeline ComputePipeline::build(VkPipelineLayout layout, const std::string& name)
{
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = _shaderStageInfo;
    pipelineInfo.layout = layout;

    VkPipeline pipeline;
    VK_ASSERT(vkCreateComputePipelines(
        _engine->device().handle(),
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &pipeline
    ));

    if (base::Log::isDebug() || !name.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(pipeline);
        nameInfo.pObjectName = name.c_str();

        setDebugUtilsObjectName(
            _engine->device().handle(),
            &nameInfo
        );
    }
    return pipeline;
}

}
}
}
}
