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

#include <bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::render::vulkan::rt {

void RayTracingSceneDataBinding::initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator)
{
    if (_engine->rayTracingSupported())
    {
        frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 }
        });
    }
}

VkDescriptorSetLayout RayTracingSceneDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE && _engine->rayTracingSupported())
    {
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;

        dsFactory.addBinding(
            0,
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
        );

        _layout = dsFactory.build(
            _engine->device().handle(),
            shaderStages
        );
    }
    return _layout;
}

VkDescriptorSet RayTracingSceneDataBinding::newDescriptorSet(
    bg2e::render::vulkan::FrameResources & frameResources,
    VkAccelerationStructureKHR tlas
) {
    if (!_engine->rayTracingSupported())
    {
        return VK_NULL_HANDLE;
    }

    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("RayTracingSceneDataBinding::newDescriptorSet() - The descriptor set layout is not created");
    }

    if (tlas == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }

    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addAccelerationStructure(0, tlas);
    ds->endUpdate();
    return ds->descriptorSet();
}

}
