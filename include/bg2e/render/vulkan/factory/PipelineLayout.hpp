
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API PipelineLayout {
public:
    PipelineLayout(Vulkan * vulkan) :_vulkan{vulkan} {}

    inline void addPushConstantRange(VkPushConstantRange pushConstant) { _pushConstantRanges.push_back(pushConstant); }
    inline void addDescriptorSetLayout(VkDescriptorSetLayout dsLayout) { _descriptorSetLayouts.push_back(dsLayout); }
    inline void addPushConstantRange(uint32_t offset, size_t size, VkShaderStageFlags stageFlags)
    {
        addPushConstantRange({ .offset = offset, .size = uint32_t(size), .stageFlags = stageFlags });
    }
    
    VkPipelineLayout build();
    
    void reset();
    
protected:
    Vulkan * _vulkan;
    std::vector<VkPushConstantRange> _pushConstantRanges;
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
    
};

}
}
}
}
