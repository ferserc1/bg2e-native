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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/geo/Vertex.hpp>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace geo {


BG2E_API VkVertexInputBindingDescription bindingDescriptionP();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPN();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPC();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPU();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNU();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNC();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNUC();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNUT();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNUUT();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsP();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPN();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPC();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPU();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNU();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNC();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUC();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUT();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUUT();



}
}
}
}
