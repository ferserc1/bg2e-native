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
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Buffer {
public:

    static Buffer* createAllocatedBuffer(
        Engine * engine,
        size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage
    );

    void cleanup();

    VkDeviceAddress deviceAddress() const;
    
    void* allocatedData();

    inline VkBuffer handle() const { return _buffer; }
    inline VmaAllocation allocation() const { return _allocation; }
    inline VmaAllocationInfo allocationInfo() const { return _info; }


protected:
    Buffer() = default;

    Engine * _engine = nullptr;

    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VmaAllocationInfo _info = {};
};

}
}
}

