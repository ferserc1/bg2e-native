
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/uniforms/materials.hpp>

namespace bg2e::scene::vk {

void ObjectDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 }
    });
}

VkDescriptorSetLayout ObjectDataBinding::createLayout()
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        
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
    
    uniforms.material = material->materialAttributes();
    
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
        ds->addImage(
            2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            material->normalTexture()
        );
        ds->addImage(
            3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            material->metalnessTexture()
        );
        ds->addImage(
            4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            material->roughnessTexture()
        );
        ds->addImage(
            5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            material->aoTexture()
        );
    ds->endUpdate();
    return ds->descriptorSet();
}

}
