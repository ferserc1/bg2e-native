/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class PhysicalDevice : public gpu::PhysicalDevice {
public:
    void choose(gpu::Instance& instance, gpu::Surface& surface) override;

    bool isValid() const override;

    const std::shared_ptr<PhysicalDeviceProperties> properties() const override;

    inline VkPhysicalDevice handle() const { return _physicalDevice; }

private:
    VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};
};

}
}
}
