#pragma once

#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace scene {
namespace vk {

class SceneData {
public:
    SceneData(bg2e::render::Vulkan * vk) : _vulkan(vk) {}
    virtual ~SceneData() = default;

    virtual void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) = 0;

    virtual VkDescriptorSetLayout createLayout() = 0;
    
    inline VkDescriptorSetLayout layout() const { return _layout; }

protected:
    bg2e::render::Vulkan * _vulkan;
    
    VkDescriptorSetLayout _layout = VK_NULL_HANDLE;

    
};

}
}
}
