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

#include <bg2e/gpu/metal/Buffer.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>

#include <cstring>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

Buffer::Buffer(metal::Device* device, const std::string& debugName)
    : gpu::Buffer(device), _device(device), _debugName(debugName)
{}

Buffer::~Buffer()
{
    cleanup();
}

#if BG2E_IS_MAC

void Buffer::cleanup()
{
    if (_buffer)
    {
        _buffer->release();
        _buffer = nullptr;
    }
}

bool Buffer::isValid() const
{
    return _buffer != nullptr;
}

static void uploadBufferData(metal::Device* device, MTL::Buffer*& outBuffer,
                             const void* data, uint64_t byteSize)
{
    if (outBuffer)
    {
        outBuffer->release();
        outBuffer = nullptr;
    }

    // Staging (shared/CPU-visible)
    MTL::Buffer* staging = device->handle()->newBuffer(byteSize, MTL::ResourceStorageModeShared);
    if (!staging)
    {
        throw std::runtime_error("metal::Buffer: failed to allocate staging buffer");
    }
    std::memcpy(staging->contents(), data, byteSize);

    // Device-local (private)
    outBuffer = device->handle()->newBuffer(byteSize, MTL::ResourceStorageModePrivate);
    if (!outBuffer)
    {
        staging->release();
        throw std::runtime_error("metal::Buffer: failed to allocate device-local buffer");
    }

    device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        MTL::CommandBuffer* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd)->handle();
        MTL::BlitCommandEncoder* blit = mtlCmd->blitCommandEncoder();
        blit->copyFromBuffer(staging, 0, outBuffer, 0, byteSize);
        blit->endEncoding();
    });

    staging->release();
}

void Buffer::createVertexBuffer(const void* data, uint64_t byteSize)
{
    uploadBufferData(_device, _buffer, data, byteSize);
    _usage    = BufferUsage::Vertex;
    _byteSize = byteSize;

    if (!_debugName.empty())
    {
        _buffer->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Buffer::createIndexBuffer(const std::vector<uint32_t>& indices)
{
    uint64_t byteSize = indices.size() * sizeof(uint32_t);
    uploadBufferData(_device, _buffer, indices.data(), byteSize);
    _usage    = BufferUsage::Index;
    _byteSize = byteSize;

    if (!_debugName.empty())
    {
        _buffer->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

#else

void Buffer::cleanup() {}

bool Buffer::isValid() const { return false; }

void Buffer::createVertexBuffer(const void*, uint64_t)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Buffer::createIndexBuffer(const std::vector<uint32_t>&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

#endif

}
}
}
