#pragma once

#include <bg2e/render/vulkan/Buffer.hpp>

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
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY
) {
    auto buffer = bg2e::render::vulkan::Buffer::createAllocatedBuffer(
        engine, sizeof(T),
        usage,
        memoryUsage
    );
    
    auto vkBuffer = buffer->handle();
    auto bufferAllocation = buffer->allocation();
    frameResources.cleanupManager.push([&, engine, vkBuffer, bufferAllocation, buffer](VkDevice) {
        engine->destroyBuffer(vkBuffer, bufferAllocation);
        delete buffer;
    });
    
    auto dataPtr = reinterpret_cast<T*>(buffer->allocatedData());
    *dataPtr = data;
    
    return buffer;
}

}
}
}
}
