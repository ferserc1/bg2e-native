
#include <bg2e/render/vulkan/Pipeline.hpp>

#include <bg2e/render/vulkan/VulkanAPI.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

Pipeline::Pipeline()
{
    // TODO: Initialize _createInfo with default values
}

Pipeline::Pipeline(const Pipeline* pipeline)
{
    // TODO: Initialize _createInfo with pipeline->_createInfo
}

void Pipeline::build(VulkanAPI *vulkanApi)
{
    _device = vulkanApi->device();

    if (!_pipelineLayout)
    {
        throw std::runtime_error("Failed to create pipeline: invalid pipeline layout set");
    }

    // Shaders

    // Dynamic state

    // Vertex input

    // Input assembly

    // Viewport anmd scissor

    // Rasterizator

    // Multisample

    // Color blend


    // Pipeline layout
    _createInfo.layout = _pipelineLayout->impl();

    // TODO: Support custom render pass and subpasses
    _createInfo.renderPass = vulkanApi->mainRenderPass();
    _createInfo.subpass = 0;

    _createInfo.basePipelineHandle = nullptr;
    _createInfo.basePipelineIndex = -1;

    // Create pipeline
    vk::Result result;
    std::tie(result, _pipeline) = _device.createGraphicsPipeline(nullptr, _createInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Destroy shader modules
}

void Pipeline::destroy()
{
    _device.destroyPipeline(_pipeline);
    _pipeline = nullptr;
}

}
}
}
