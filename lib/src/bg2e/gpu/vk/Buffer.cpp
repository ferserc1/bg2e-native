/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/vk/Buffer.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>

#include <cstring>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

Buffer::Buffer(vk::Device* device, const std::string& debugName)
    : gpu::Buffer(device), _device(device), _debugName(debugName)
{}

Buffer::~Buffer()
{
    cleanup();
}

void Buffer::cleanup()
{
    if (_buffer != VK_NULL_HANDLE && _device != nullptr)
    {
        vmaDestroyBuffer(_device->allocator(), _buffer, _allocation);
        _buffer     = VK_NULL_HANDLE;
        _allocation = VK_NULL_HANDLE;
        _mappedData = nullptr;
    }
}

void Buffer::createVertexBuffer(const void* data, uint64_t byteSize)
{
    VkBufferUsageFlags vkUsage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT     |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT      |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT    |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    // When ray tracing is available, also allow this buffer to be used as
    // acceleration structure build geometry input (RayTracingMesh).
    if (_device->rayTracingEnabled())
    {
        vkUsage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }

    uploadWithStaging(data, byteSize, vkUsage, BufferUsage::Vertex);
}

void Buffer::createIndexBuffer(const std::vector<uint32_t>& indices)
{
    uint64_t byteSize = indices.size() * sizeof(uint32_t);

    VkBufferUsageFlags vkUsage =
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT      |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT      |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT    |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (_device->rayTracingEnabled())
    {
        vkUsage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }

    uploadWithStaging(indices.data(), byteSize, vkUsage, BufferUsage::Index);
}

VkDeviceAddress Buffer::deviceAddress() const
{
    if (_buffer == VK_NULL_HANDLE)
    {
        return 0;
    }
    VkBufferDeviceAddressInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = _buffer;
    return vkGetBufferDeviceAddress(_device->handle(), &info);
}

void Buffer::createUniformBuffer(const void* data, uint64_t byteSize)
{
    createMappedBuffer(data, byteSize,
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       BufferUsage::Uniform, "createUniformBuffer");
}

void Buffer::createStorageBuffer(const void* data, uint64_t byteSize)
{
    createMappedBuffer(data, byteSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       BufferUsage::Storage, "createStorageBuffer");
}

void Buffer::updateUniformBuffer(const void* data, uint64_t byteSize)
{
    updateMappedBuffer(data, byteSize, "updateUniformBuffer");
}

void Buffer::updateStorageBuffer(const void* data, uint64_t byteSize)
{
    updateMappedBuffer(data, byteSize, "updateStorageBuffer");
}

void Buffer::createMappedBuffer(const void* data, uint64_t byteSize,
                                VkBufferUsageFlags vkUsage, BufferUsage gpuUsage,
                                const char* debugTag)
{
    cleanup();

    // Host-visible, persistently mapped buffer. Suitable for small dynamic data
    // updated every frame (e.g. per-object matrices); no staging copy needed.
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = byteSize;
    bufInfo.usage = vkUsage;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocOut{};
    VkResult result = vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                                      &_buffer, &_allocation, &allocOut);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("vk::Buffer::") + debugTag + ": vmaCreateBuffer failed");
    }

    _mappedData = allocOut.pMappedData;
    if (data != nullptr && _mappedData != nullptr)
    {
        std::memcpy(_mappedData, data, byteSize);
    }

    _usage    = gpuUsage;
    _byteSize = byteSize;
}

void Buffer::updateMappedBuffer(const void* data, uint64_t byteSize, const char* debugTag)
{
    if (_mappedData == nullptr)
    {
        throw std::runtime_error(std::string("vk::Buffer::") + debugTag +
            ": buffer is not host-visible (was it created with createUniformBuffer/createStorageBuffer?)");
    }
    if (byteSize > _byteSize)
    {
        throw std::runtime_error(std::string("vk::Buffer::") + debugTag +
            ": update size exceeds the buffer capacity");
    }
    std::memcpy(_mappedData, data, byteSize);
}

void Buffer::uploadWithStaging(const void* data, uint64_t byteSize,
                               VkBufferUsageFlags vkUsage, BufferUsage gpuUsage)
{
    cleanup();

    // Staging buffer: CPU-writable, used only for the upload
    VkBuffer      stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc  = VK_NULL_HANDLE;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size  = byteSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocCI{};
    stagingAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    stagingAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingAllocOut{};
    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &stagingInfo, &stagingAllocCI,
                              &stagingBuffer, &stagingAlloc, &stagingAllocOut));

    std::memcpy(stagingAllocOut.pMappedData, data, byteSize);

    // Device-local buffer
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = byteSize;
    bufInfo.usage = vkUsage;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage         = VMA_MEMORY_USAGE_AUTO;
    allocCI.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VK_ASSERT(vmaCreateBuffer(_device->allocator(), &bufInfo, &allocCI,
                              &_buffer, &_allocation, nullptr));

    // Upload via immediate submit
    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        VkCommandBuffer raw = dynamic_cast<vk::CommandBuffer*>(cmd)->handle();

        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = 0;
        region.size      = byteSize;
        vkCmdCopyBuffer(raw, stagingBuffer, _buffer, 1, &region);
    });

    vmaDestroyBuffer(_device->allocator(), stagingBuffer, stagingAlloc);

    _usage    = gpuUsage;
    _byteSize = byteSize;
}

}
}
}
