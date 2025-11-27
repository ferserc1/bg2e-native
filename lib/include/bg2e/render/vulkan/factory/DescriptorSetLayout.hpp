#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API DescriptorSetLayout {
public:
    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(
        VkDevice device,
        VkShaderStageFlags shaderStages,
        void* pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0
    );

protected:
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
};

}
}
}
}
