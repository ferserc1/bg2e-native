
#pragma once

#include <bg2e/scene/vk/SceneData.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/vulkan/Image.hpp>

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API ObjectData : public SceneData {
public:
    struct ObjectUniforms
    {
        glm::mat4 modelMatrix;
        
        // TODO: add material uniforms
    };

    ObjectData(bg2e::render::Vulkan *);
    
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
