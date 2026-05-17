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

#include <bg2e/scene/vk/LightDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::scene::vk {

void LightDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BG2E_MAX_FORWARD_LIGHTS }
    });
}

VkDescriptorSetLayout LightDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE)
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;

        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        _layout = dsFactory.build(_engine->device().handle(), shaderStages);
    }
    return _layout;
}

VkDescriptorSet LightDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    const LightUniforms & lights
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("LightDataBinding::newDescriptorSet() - The descriptor set layout is not created");
    }

    auto uniformBuffer = bg2e::render::vulkan::macros::createBuffer(_engine, frameResources, lights);
    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addBuffer(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            uniformBuffer, sizeof(LightUniforms), 0
        );
    ds->endUpdate();
    return ds->descriptorSet();
}

}
