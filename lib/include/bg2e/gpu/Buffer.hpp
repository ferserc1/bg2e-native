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

protected:
    BufferUsage _usage    = BufferUsage::None;
    uint64_t    _byteSize = 0;
};

}
}
