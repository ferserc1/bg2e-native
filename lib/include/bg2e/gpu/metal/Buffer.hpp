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

#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <string>

namespace bg2e {
namespace gpu {
namespace metal {

class Device;

class Buffer : public gpu::Buffer {
public:
    Buffer(metal::Device* device, const std::string& debugName = {});
    ~Buffer() override;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void cleanup() override;
    bool isValid() const override;

    void createVertexBuffer(const void* data, uint64_t byteSize) override;
    void createIndexBuffer(const std::vector<uint32_t>& indices) override;

    void createUniformBuffer(const void* data, uint64_t byteSize) override;
    void createStorageBuffer(const void* data, uint64_t byteSize) override;
    void updateUniformBuffer(const void* data, uint64_t byteSize) override;
    void updateStorageBuffer(const void* data, uint64_t byteSize) override;

#if BG2E_IS_MAC
    MTL::Buffer* handle() const { return _buffer; }
#endif

private:
    metal::Device* _device = nullptr;
    std::string    _debugName;

#if BG2E_IS_MAC
    MTL::Buffer* _buffer = nullptr;
#endif
};

}
}
}
