
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/ShaderModule.hpp>

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

VkPipeline ComputePipeline::build(VkPipelineLayout layout)
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
    return pipeline;
}

}
}
}
}
