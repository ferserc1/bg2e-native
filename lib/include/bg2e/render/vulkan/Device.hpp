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
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Surface.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Device {
public:

	void create(const Instance& instance, const PhysicalDevice& physicalDevice, bool offscreen);

	void cleanup();

	inline VkDevice handle() const { return _device; }
	inline bool isValid() const { return _device != VK_NULL_HANDLE; }

    inline VkQueue graphicsQueue() const { return _graphicsQueue; }
    inline VkQueue presentQueue() const { return _presentQueue; }
    inline uint32_t graphicsFamily() const { return _graphicsFamily; }
    inline uint32_t presentFamily() const { return _presentFamily; }
    
    void waitIdle() const;

protected:
	VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    uint32_t _graphicsFamily = 0;
    uint32_t _presentFamily = 0;
};

}
}
}
