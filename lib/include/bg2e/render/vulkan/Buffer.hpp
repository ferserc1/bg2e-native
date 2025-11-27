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

