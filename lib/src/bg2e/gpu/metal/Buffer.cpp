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
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>

#include <cstring>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

Buffer::Buffer(metal::Device* device, const std::string& debugName)
    : gpu::Buffer(device), _device(device), _debugName(debugName)
{
    requireDebugName(debugName);
}

Buffer::~Buffer()
{
    cleanup();
}

#if BG2E_IS_MAC

void Buffer::cleanup()
{
    detail::DiagnosticScope diagnosticScope("metal", "Buffer.cleanup", _diagnosticIdentity.value);
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
                             const void* data, uint64_t byteSize, const std::string& debugName)
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
    staging->setLabel(NS::String::string((debugName + "-upload-staging-buffer").c_str(), NS::UTF8StringEncoding));
    std::memcpy(staging->contents(), data, byteSize);

    // Device-local (private)
    outBuffer = device->handle()->newBuffer(byteSize, MTL::ResourceStorageModePrivate);
    if (!outBuffer)
    {
        staging->release();
        throw std::runtime_error("metal::Buffer: failed to allocate device-local buffer");
    }

    outBuffer->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
                            {
                                MTL::CommandBuffer* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd)->handle();
                                MTL::BlitCommandEncoder* blit = mtlCmd->blitCommandEncoder();
                                blit->setLabel(NS::String::string((debugName + "-upload-encoder").c_str(), NS::UTF8StringEncoding));
                                blit->copyFromBuffer(staging, 0, outBuffer, 0, byteSize);
                                blit->endEncoding();
                            });

    staging->release();
}

void Buffer::createVertexBuffer(const void* data, uint64_t byteSize)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createVertexBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    uploadBufferData(_device, _buffer, data, byteSize, _debugName);
    _usage = BufferUsage::Vertex;
    _byteSize = byteSize;

    if (!_debugName.empty())
    {
        _buffer->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Buffer::createIndexBuffer(const std::vector<uint32_t>& indices)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createIndexBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    uint64_t byteSize = indices.size() * sizeof(uint32_t);
    uploadBufferData(_device, _buffer, indices.data(), byteSize, _debugName);
    _usage = BufferUsage::Index;
    _byteSize = byteSize;

    if (!_debugName.empty())
    {
        _buffer->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

// Uniform/storage buffers use a CPU-visible (shared) allocation so that the
// contents can be updated directly each frame without a staging copy.
static void createSharedBuffer(metal::Device* device, MTL::Buffer*& outBuffer,
                               const void* data, uint64_t byteSize, const std::string& debugName)
{
    if (outBuffer)
    {
        outBuffer->release();
        outBuffer = nullptr;
    }

    outBuffer = device->handle()->newBuffer(byteSize, MTL::ResourceStorageModeShared);
    if (!outBuffer)
    {
        throw std::runtime_error("metal::Buffer: failed to allocate shared buffer");
    }
    outBuffer->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    if (data)
    {
        std::memcpy(outBuffer->contents(), data, byteSize);
    }
}

void Buffer::createUniformBuffer(const void* data, uint64_t byteSize)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createUniformBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    createSharedBuffer(_device, _buffer, data, byteSize, _debugName);
    _usage = BufferUsage::Uniform;
    _byteSize = byteSize;

    if (!_debugName.empty())
    {
        _buffer->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Buffer::createStorageBuffer(const void* data, uint64_t byteSize)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createStorageBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    base::CpuProfileScope profile("metal.buffer.createStorage");
    createSharedBuffer(_device, _buffer, data, byteSize, _debugName);
    _usage = BufferUsage::Storage;
    _byteSize = byteSize;

    if (!_debugName.empty())
    {
        _buffer->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Buffer::updateUniformBuffer(const void* data, uint64_t byteSize)
{
    if (!_buffer)
    {
        throw std::runtime_error("metal::Buffer::updateUniformBuffer: buffer not created");
    }
    if (byteSize > _byteSize)
    {
        throw std::runtime_error("metal::Buffer::updateUniformBuffer: update size exceeds the buffer capacity");
    }
    std::memcpy(_buffer->contents(), data, byteSize);
}

void Buffer::updateStorageBuffer(const void* data, uint64_t byteSize)
{
    if (!_buffer)
    {
        throw std::runtime_error("metal::Buffer::updateStorageBuffer: buffer not created");
    }
    if (byteSize > _byteSize)
    {
        throw std::runtime_error("metal::Buffer::updateStorageBuffer: update size exceeds the buffer capacity");
    }
    std::memcpy(_buffer->contents(), data, byteSize);
}

#else

void Buffer::cleanup()
{
    detail::DiagnosticScope diagnosticScope("metal", "Buffer.cleanup", _diagnosticIdentity.value);
}

bool Buffer::isValid() const
{
    return false;
}

void Buffer::createVertexBuffer(const void*, uint64_t)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createVertexBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Buffer::createIndexBuffer(const std::vector<uint32_t>&)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createIndexBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Buffer::createUniformBuffer(const void*, uint64_t)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createUniformBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Buffer::createStorageBuffer(const void*, uint64_t)
{
    detail::DiagnosticScope resourceScope("metal", "buffer.createStorageBuffer", _diagnosticIdentity.value);
    bg2e_deep_trace("metal", "buffer.create", "resource=" << _diagnosticIdentity.value << " name=" << _debugName);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Buffer::updateUniformBuffer(const void*, uint64_t)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Buffer::updateStorageBuffer(const void*, uint64_t)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

#endif

}
}
}
