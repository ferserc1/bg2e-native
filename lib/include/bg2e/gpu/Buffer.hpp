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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>

#include <vector>
#include <stdexcept>

namespace bg2e {
namespace gpu {

class BG2E_API Buffer : public DeviceResource {
public:
    explicit Buffer(Device* device) : DeviceResource(device) {}
    virtual ~Buffer() = default;

    uint64_t    byteSize() const { return _byteSize; }
    BufferUsage usage()    const { return _usage; }

    virtual void createVertexBuffer(const void* data, uint64_t byteSize)
    {
        throw std::runtime_error("Buffer::createVertexBuffer not implemented");
    }

    virtual void createIndexBuffer(const std::vector<uint32_t>& indices)
    {
        throw std::runtime_error("Buffer::createIndexBuffer not implemented");
    }

    template <typename VertexT>
    void createVertexBuffer(const std::vector<VertexT>& vertices)
    {
        createVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexT));
    }

    // --- Uniform / storage buffers -------------------------------------------
    // Uniform and storage buffers created here are meant for small, frequently
    // updated data (e.g. per-frame matrices). The backend chooses an appropriate
    // memory strategy (host-visible / persistently mapped on Vulkan, shared
    // storage mode on Metal); the public API only declares intent.

    virtual void createUniformBuffer(const void* data, uint64_t byteSize)
    {
        throw std::runtime_error("Buffer::createUniformBuffer not implemented");
    }

    virtual void createStorageBuffer(const void* data, uint64_t byteSize)
    {
        throw std::runtime_error("Buffer::createStorageBuffer not implemented");
    }

    // Re-upload data into a buffer previously created with createUniformBuffer /
    // createStorageBuffer. Intended for buffers duplicated per frame through
    // FrameResourceRing; the new data must not exceed the original byte size.
    virtual void updateUniformBuffer(const void* data, uint64_t byteSize)
    {
        throw std::runtime_error("Buffer::updateUniformBuffer not implemented");
    }

    virtual void updateStorageBuffer(const void* data, uint64_t byteSize)
    {
        throw std::runtime_error("Buffer::updateStorageBuffer not implemented");
    }

    template <typename T>
    void createUniformBuffer(const T& data)
    {
        createUniformBuffer(&data, sizeof(T));
    }

    template <typename T>
    void createStorageBuffer(const std::vector<T>& data)
    {
        createStorageBuffer(data.data(), data.size() * sizeof(T));
    }

    template <typename T>
    void updateUniformBuffer(const T& data)
    {
        updateUniformBuffer(&data, sizeof(T));
    }

    template <typename T>
    void updateStorageBuffer(const std::vector<T>& data)
    {
        updateStorageBuffer(data.data(), data.size() * sizeof(T));
    }

protected:
    BufferUsage _usage    = BufferUsage::None;
    uint64_t    _byteSize = 0;
};

}
}
