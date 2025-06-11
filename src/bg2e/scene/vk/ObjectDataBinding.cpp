
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/uniforms/materials.hpp>

namespace bg2e::scene::vk {

void ObjectDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        
        // TODO: Update the number of combined image samplers
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
    });
}

VkDescriptorSetLayout ObjectDataBinding::createLayout()
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        
        // TODO: Add other textures
        
        _layout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    
    return _layout;
}

VkDescriptorSet ObjectDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    bg2e::render::MaterialBase * material,
    const glm::mat4 & modelMatrix
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("ObjectData::newDescriptorSet() - The descriptor set layout is not created");
    }
    
    ObjectUniforms uniforms;
    
    uniforms.modelMatrix = modelMatrix;
    
    // TODO: Add uniform buffer data from material
    uniforms.material.albedo = material->materialAttributes().albedo();
    
    auto uniformBuffer = bg2e::render::vulkan::macros::createBuffer(_engine, frameResources, uniforms);
    
    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addBuffer(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            uniformBuffer, sizeof(ObjectUniforms), 0
        );
        ds->addImage(
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            material->albedoTexture()
        );
        // TODO: Add textures
        
    ds->endUpdate();
    return ds->descriptorSet();
}

}
