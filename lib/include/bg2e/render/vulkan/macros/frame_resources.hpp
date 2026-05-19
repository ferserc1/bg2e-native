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

#include <bg2e/render/vulkan/Buffer.hpp>
#include <string>

namespace bg2e {
namespace render {
namespace vulkan {
namespace macros {

// Create a buffer associated to this frame resources. The Vulkan buffer
// will be released when the frame is done. The Buffer object heap memory
// will be released automatically by the frameResources. Do not try to
// store the Buffer object in any kind of smart pointer
template <typename T>
Buffer* createBuffer(
    Engine * engine,
    FrameResources& frameResources,
    const T& data,
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    const std::string & name = ""
) {
    auto buffer = bg2e::render::vulkan::Buffer::createAllocatedBuffer(
        engine, sizeof(T),
        usage,
        memoryUsage,
        name
    );
    
    auto vkBuffer = buffer->handle();
    auto bufferAllocation = buffer->allocation();
    frameResources.cleanupManager.push([&, engine, vkBuffer, bufferAllocation, buffer](VkDevice) {
        delete buffer;
    });
    
    auto dataPtr = reinterpret_cast<T*>(buffer->allocatedData());
    *dataPtr = data;
    buffer->flushAllocatedData();
    
    return buffer;
}

template <typename T>
Buffer* createBuffer(
    Engine * engine,
    FrameResources& frameResources,
    const std::vector<T>& data,
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    const std::string & name = ""
) {
    const VkDeviceSize bufferSize = sizeof(T) * data.size();

    if (bufferSize == 0)
    {
        throw std::runtime_error("createBuffer(vector) - Cannot create a zero-sized buffer");
    }

    auto buffer = bg2e::render::vulkan::Buffer::createAllocatedBuffer(
        engine,
        bufferSize,
        usage,
        memoryUsage,
        name
    );

    auto vkBuffer = buffer->handle();
    auto bufferAllocation = buffer->allocation();

    frameResources.cleanupManager.push([&, engine, vkBuffer, bufferAllocation, buffer](VkDevice) {
        delete buffer;
    });

    std::memcpy(
        buffer->allocatedData(),
        data.data(),
        static_cast<size_t>(bufferSize)
    );

    buffer->flushAllocatedData();

    return buffer;
}

}
}
}
}
