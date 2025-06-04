
#pragma once

#include <bg2e/scene/vk/SceneDataBinding.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/MaterialBase.hpp>

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API ObjectDataBinding : public SceneDataBinding {
public:
    struct ObjectUniforms
    {
        glm::mat4 modelMatrix;
        
        // TODO: add material uniforms
    };

    ObjectDataBinding(bg2e::render::Engine * engine) :SceneDataBinding(engine) {}
    
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;
    
    // The invoker of this function is the owner of the Set Descriptor and shall be responsible for deleting it.
    VkDescriptorSetLayout createLayout() override;
    
    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        bg2e::render::MaterialBase * material,
        const glm::mat4& modelMatrix
    );
};

}
}
}
