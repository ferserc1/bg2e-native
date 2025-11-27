#include <bg2e/scene/vk/FrameDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::scene::vk {

void FrameDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    });
}
    
VkDescriptorSetLayout FrameDataBinding::createLayout()
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        
        _layout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_VERTEX_BIT);
    }
    return _layout;
}

VkDescriptorSet FrameDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projectionMatrix
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("FrameData::newDescriptorSet() - The descriptor set layout is not created");
    }
    
    SceneUniforms uniforms {
        viewMatrix,
        projectionMatrix
    };
    
    auto uniformBuffer = bg2e::render::vulkan::macros::createBuffer(_engine, frameResources, uniforms);
    
    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addBuffer(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            uniformBuffer, sizeof(SceneUniforms), 0
        );
    ds->endUpdate();
    return ds->descriptorSet();
}

}
