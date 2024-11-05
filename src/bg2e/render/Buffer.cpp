
#include <bg2e/render/Buffer.hpp>

namespace bg2e {
namespace render {

Buffer* Buffer::createAllocatedBuffer(
    Vulkan * vulkan,
    size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage
) {
    auto buffer = new Buffer();
    
    buffer->_vulkan = vulkan;
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VK_ASSERT(vmaCreateBuffer(
        vulkan->allocator(),
        &bufferInfo,
        &allocInfo,
        &buffer->_buffer,
        &buffer->_allocation,
        &buffer->_info
    ));
    
    return buffer;
}

void Buffer::cleanup()
{
    vmaDestroyBuffer(_vulkan->allocator(), _buffer, _allocation);
    _buffer = VK_NULL_HANDLE;
    _allocation = VK_NULL_HANDLE;
    _info = {};
}

void* Buffer::allocatedData()
{
    return getMappedData(_allocation);
}

VkDeviceAddress Buffer::deviceAddress() const
{
    if (_buffer == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Buffer::deviceAddress() called on a null buffer");
    }

    VkBufferDeviceAddressInfo deviceAddressInfo = {};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = _buffer;
    return vkGetBufferDeviceAddress(_vulkan->device(), &deviceAddressInfo);
}
    
}
}
