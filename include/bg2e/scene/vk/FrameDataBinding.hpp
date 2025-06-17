
#pragma once

#include <bg2e/scene/vk/SceneDataBinding.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API FrameDataBinding : public SceneDataBinding {
public:
    struct SceneUniforms {
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
    };
    
    FrameDataBinding(bg2e::render::Engine * engine) :SceneDataBinding(engine) {}
    
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;
    
    VkDescriptorSetLayout createLayout() override;
    
    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projectionMatrix
    );
};

}
}
}


