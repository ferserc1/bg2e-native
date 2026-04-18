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
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_core.h>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

struct BG2E_API RayTracingSceneInstance {
    uint64_t blasDeviceAddress = 0;
    glm::mat4 transform = glm::mat4(1.0f);
    uint32_t instanceCustomIndex = 0;
    uint8_t mask = 0xFF;
    uint32_t stbRecordOffset = 0;
    VkGeometryInstanceFlagsKHR flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
};

}
}
}
}
