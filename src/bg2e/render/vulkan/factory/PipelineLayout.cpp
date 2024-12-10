
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e::render::vulkan::factory {

VkPipelineLayout PipelineLayout::build()
{
    auto layoutInfo = Info::pipelineLayoutInfo();
    if (_pushConstantRanges.size() > 0)
    {
        layoutInfo.pPushConstantRanges = _pushConstantRanges.data();
        layoutInfo.pushConstantRangeCount = uint32_t(_pushConstantRanges.size());
    }
    
    if (_descriptorSetLayouts.size() > 0)
    {
        layoutInfo.pSetLayouts = _descriptorSetLayouts.data();
        layoutInfo.setLayoutCount = uint32_t(_descriptorSetLayouts.size());
    }

    VkPipelineLayout result = VK_NULL_HANDLE;
    VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &result));
    return result;
}

void PipelineLayout::reset()
{
    _pushConstantRanges.clear();
    _descriptorSetLayouts.clear();
}

}
