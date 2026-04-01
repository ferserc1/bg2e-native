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

#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e::render::vulkan::factory {

void DescriptorSetLayout::addBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding bindingInfo = {};
    bindingInfo.binding = binding;
    bindingInfo.descriptorCount = 1;
    bindingInfo.descriptorType = type;
    _bindings.push_back(bindingInfo);
}

void DescriptorSetLayout::clear()
{
    _bindings.clear();
}

VkDescriptorSetLayout DescriptorSetLayout::build(VkDevice device, VkShaderStageFlags shaderStages, void* /*pNext*/, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : _bindings)
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = uint32_t(_bindings.size());
    layoutInfo.pBindings = _bindings.data();
    layoutInfo.flags = flags;
    VkDescriptorSetLayout set;
    VK_ASSERT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &set));
    return set;
}

}
