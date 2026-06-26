/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/uniforms/materials.hpp>

namespace bg2e::scene::vk {

void ObjectDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6 }
    });
}

VkDescriptorSetLayout ObjectDataBinding::createLayout(VkShaderStageFlags shaderStages)
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
        dsFactory.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        
        _layout = dsFactory.build(_engine->device().handle(), shaderStages);
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
        ds->addImage(
            6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            material->lightEmissionTexture()
        );
    ds->endUpdate();
    return ds->descriptorSet();
}

}
