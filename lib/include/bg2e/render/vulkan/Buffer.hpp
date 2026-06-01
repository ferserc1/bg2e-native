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
    virtual ~Buffer();

    Buffer(
        Engine * engine,
        VkBuffer buffer,
        VmaAllocation allocation,
        VmaAllocationInfo info,
        const std::string & name = ""
    );

    static Buffer* createAllocatedBuffer(
        Engine * engine,
        size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage,
        const std::string & name = ""
    );

    void cleanup();

    [[nodiscard]] VkDeviceAddress deviceAddress() const;
    
    void* allocatedData();
    void flushAllocatedData();

    [[nodiscard]] VkBuffer handle() const { return _buffer; }
    [[nodiscard]] VmaAllocation allocation() const { return _allocation; }
    [[nodiscard]] VmaAllocationInfo allocationInfo() const { return _info; }
    [[nodiscard]] size_t logicSize() const { return _logicSize; }

    [[nodiscard]] const std::string & debugName() const { return _debugName; }
protected:
    Buffer() = default;

    Engine * _engine = nullptr;

    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VmaAllocationInfo _info = {};
    size_t _logicSize = 0;

    std::string _debugName;
};

}
}
}

