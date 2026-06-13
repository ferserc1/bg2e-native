/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/vk/VertexDescriptor.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

VkFormat VertexDescriptor::toVkVertexFormat(gpu::Format format)
{
    switch (format)
    {
        case gpu::Format::R32_SFLOAT:          return VK_FORMAT_R32_SFLOAT;
        case gpu::Format::R32G32_SFLOAT:       return VK_FORMAT_R32G32_SFLOAT;
        case gpu::Format::R32G32B32_SFLOAT:    return VK_FORMAT_R32G32B32_SFLOAT;
        case gpu::Format::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:                               return VK_FORMAT_UNDEFINED;
    }
}

void VertexDescriptor::buildVkVertexInput(
    const std::vector<gpu::VertexBufferDescription>&  descriptions,
    std::vector<VkVertexInputBindingDescription>&     outBindings,
    std::vector<VkVertexInputAttributeDescription>&   outAttributes)
{
    outBindings.clear();
    outAttributes.clear();

    for (const auto& desc : descriptions)
    {
        VkVertexInputBindingDescription binding{};
        binding.binding   = desc.binding;
        binding.stride    = desc.stride;
        binding.inputRate = (desc.inputRate == gpu::VertexInputRate::Vertex)
                          ? VK_VERTEX_INPUT_RATE_VERTEX
                          : VK_VERTEX_INPUT_RATE_INSTANCE;
        outBindings.push_back(binding);

        for (const auto& attr : desc.attributes)
        {
            VkVertexInputAttributeDescription vkAttr{};
            vkAttr.location = attr.location;
            vkAttr.binding  = attr.binding;
            vkAttr.format   = toVkVertexFormat(attr.format);
            vkAttr.offset   = attr.offset;
            outAttributes.push_back(vkAttr);
        }
    }
}

}
}
}
