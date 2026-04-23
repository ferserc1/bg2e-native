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
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/Node.hpp>

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

class BG2E_API RayTracingScene {
public:
    RayTracingScene(Engine * engine);

    bool update(VkCommandBuffer cmd, scene::Node * node);
    void cleanup();

    [[nodiscard]] VkAccelerationStructureKHR tlas() const { return _tlas; }
    [[nodiscard]] uint64_t deviceAddress() const { return _tlasDeviceAddress; }

protected:
    render::Engine* _engine = nullptr;

    std::unique_ptr<Buffer> _instanceBuffer;
    std::unique_ptr<Buffer> _tlasBuffer;

    // We need to store the scratch buffer because the build TLAS command is added
    // to the command buffer and the TLAS build process is asyncrhonous
    std::unique_ptr<Buffer> _scratchBuffer;

    VkAccelerationStructureKHR _tlas = VK_NULL_HANDLE;
    uint64_t _tlasDeviceAddress = 0;
};

}
}
}
}
