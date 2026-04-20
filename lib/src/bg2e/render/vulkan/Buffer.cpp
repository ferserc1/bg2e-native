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

#include <bg2e/render/vulkan/Buffer.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

Buffer* Buffer::createAllocatedBuffer(
    Engine * engine,
    size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage
) {
    auto buffer = new Buffer();

    buffer->_engine = engine;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VK_ASSERT(vmaCreateBuffer(
        engine->allocator(),
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
    if (_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(_engine->allocator(), _buffer, _allocation);
    }
    _buffer = VK_NULL_HANDLE;
    _allocation = VK_NULL_HANDLE;
    _info = {};
}

void* Buffer::allocatedData()
{
    return getMappedData(_allocation);
}

void Buffer::flushAllocatedData()
{
    vmaFlushAllocation(
        _engine->allocator(),
        _allocation,
        0,
        VK_WHOLE_SIZE
    );
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
    return vkGetBufferDeviceAddress(_engine->device().handle(), &deviceAddressInfo);
}

}
}
}

