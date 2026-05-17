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

#include <bg2e/scene/vk/EnvironmentDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::scene::vk {

void EnvironmentDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    });
}

VkDescriptorSetLayout EnvironmentDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        
        _layout = dsFactory.build(_engine->device().handle(), shaderStages);
    }
    return _layout;
}

VkDescriptorSet EnvironmentDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    bg2e::render::EnvironmentResources * environmentResources
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("EnvironmentData::newDescriptorSet() - The descriptor set layout is not created");
    }
    
    EnvironmentUniforms envUniforms;
    envUniforms.maxEnvMapLod = static_cast<float>(environmentResources->specularReflectionMapImage()->mipLevels());
    auto uniformBuffer = bg2e::render::vulkan::macros::createBuffer(_engine, frameResources, envUniforms);

    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addImage(
            0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->irradianceMapImage()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->irradianceMapSampler()
        );
        
        ds->addImage(
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->specularReflectionMapImage()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->specularReflectionMapSampler()
        );
        
        ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            environmentResources->brdfIntegrationMapImage()->image()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            environmentResources->brdfIntegrationMapSampler()
        );

        ds->addBuffer(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            uniformBuffer, sizeof(EnvironmentUniforms), 0
        );

    ds->endUpdate();
    return ds->descriptorSet();
}


}
