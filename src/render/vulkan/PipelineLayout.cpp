
#include <bg2e/render/vulkan/PipelineLayout.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

PipelineLayout::PipelineLayout()
{
    
}

void PipelineLayout::build(VulkanAPI *vulkanApi)
{
    _device = vulkanApi->device();

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;

    // TODO: specify layout
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    _impl = _device.createPipelineLayout(pipelineLayoutInfo);
}

void PipelineLayout::destroy()
{
    _device.destroyPipelineLayout(_impl);
}

}
}
}
