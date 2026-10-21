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

#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <algorithm>
#include <limits>

namespace bg2e::draw::pipeline {
namespace {
bool retired(const std::shared_ptr<const gpu::detail::RecordingCompletion>& completion)
{
    return completion && completion->retired();
}

uint64_t imageBytes(const gpu::ImageDescription& description)
{
    uint64_t bytes = 0;
    uint32_t width = description.size.width, height = description.size.height;
    for (uint32_t level = 0; level < gpu::effectiveMipLevels(description); ++level)
    {
        const uint64_t pixels = uint64_t(width) * height;
        const uint64_t stride = gpu::pixelFormatBytes(description.format) * (description.type == gpu::ImageType::Cubemap ? 6u : 1u);
        if (!stride || pixels > (std::numeric_limits<uint64_t>::max() - bytes) / stride)
        {
            throw std::length_error("Temporary image byte accounting overflow");
        }
        bytes += pixels * stride;
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    return bytes;
}
}

const std::shared_ptr<gpu::ResourceSet>& TemporaryBindings::resources() const
{
    return _resources;
}

const std::shared_ptr<gpu::Buffer>& TemporaryBindings::uniforms() const
{
    return _uniforms;
}

void TemporaryBindings::writeUniforms(const void* data, uint64_t bytes)
{
    if (!data || !bytes || bytes != _uniformBytes || !_uniforms)
    {
        throw std::invalid_argument("Temporary uniforms must match the leased buffer size");
    }
    if (!_uniforms->isValid())
    {
        _tuple.clear();
        _uniforms->createUniformBuffer(data, bytes);
        base::CpuProfileScope::count("temporary.uniformAllocations", 1, "allocations");
    }
    else
    {
        _uniforms->updateUniformBuffer(data, bytes);
    }
}

bool TemporaryBindings::changed(gpu::ShaderBinding binding, gpu::ResourceType type,
    gpu::DeviceResource* resource, uint64_t generation)
{
    if (!resource || !resource->isValid() || resource->device() != _layout->device())
    {
        throw std::invalid_argument("Temporary bindings require valid same-device resources");
    }
    auto owner = resource->weak_from_this().lock();
    const auto found = std::find_if(_tuple.begin(), _tuple.end(), [&](const Binding& item)
    {
        return item.type == type && item.binding.vulkan == binding.vulkan && item.binding.metal == binding.metal;
    });
    const bool different = found == _tuple.end() || found->resource != resource || found->generation != generation ||
        found->owner.expired();
    if (owner && std::find(_owners.begin(), _owners.end(), owner) == _owners.end())
    {
        _owners.push_back(owner);
    }
    if (different)
    {
        Binding item{binding, type, resource, generation, owner};
        if (found == _tuple.end())
        {
            _tuple.push_back(item);
        }
        else
        {
            *found = item;
        }
        _dirty = true;
    }
    return different;
}

void TemporaryBindings::setUniformBuffer(gpu::ShaderBinding binding, gpu::Buffer* buffer)
{
    // Borrowed uniform buffers may be recreated at the same address. Rebind them
    // conservatively; internally leased uniform buffers keep stable native storage.
    const bool different = changed(binding, gpu::ResourceType::UniformBuffer, buffer, 0);
    if (different || buffer != _uniforms.get())
    {
        try
        {
            _resources->setUniformBuffer(binding, buffer);
        }
        catch (...)
        {
            _tuple.clear();
            throw;
        }
        _dirty = true;
    }
}

void TemporaryBindings::setSampledImage(gpu::ShaderBinding binding, gpu::Image* image)
{
    if (changed(binding, gpu::ResourceType::SampledImage, image, image ? image->bindingGeneration() : 0))
    {
        try
        {
            _resources->setSampledImage(binding, image);
        }
        catch (...)
        {
            _tuple.clear();
            throw;
        }
    }
}

void TemporaryBindings::setStorageImage(gpu::ShaderBinding binding, gpu::Image* image)
{
    if (changed(binding, gpu::ResourceType::StorageImage, image, image ? image->bindingGeneration() : 0))
    {
        try
        {
            _resources->setStorageImage(binding, image);
        }
        catch (...)
        {
            _tuple.clear();
            throw;
        }
    }
}

void TemporaryBindings::setSampler(gpu::ShaderBinding binding, gpu::Sampler* sampler)
{
    if (changed(binding, gpu::ResourceType::Sampler, sampler, 0))
    {
        try
        {
            _resources->setSampler(binding, sampler);
        }
        catch (...)
        {
            _tuple.clear();
            throw;
        }
    }
}

void TemporaryBindings::update()
{
    if (_dirty)
    {
        try
        {
            _resources->update();
            _dirty = false;
            base::CpuProfileScope::count("temporary.bindingUpdates", 1);
        }
        catch (...)
        {
            _tuple.clear(); // Failed native updates cannot certify tuple equality.
            throw;
        }
    }
    else
    {
        base::CpuProfileScope::count("temporary.unchangedBindings", 1);
    }
}

void TemporaryBindings::releaseOwners()
{
    _owners.clear();
}

TemporaryResourcePool::TemporaryResourcePool(gpu::Device& device, TemporaryPoolLimits limits)
    : _device(&device), _limits(limits)
{
    if (!device.isValid())
    {
        throw std::invalid_argument("Temporary pool requires a live device");
    }
}

void TemporaryResourcePool::setEnabled(bool enabled)
{
    _enabled = enabled;
}

void TemporaryResourcePool::collect()
{
    ++_epoch;
    for (auto& entry : _bindings)
    {
        if (entry.use_count() == 1 && entry->_resources.use_count() == 1 && retired(entry->_completion))
        {
            entry->releaseOwners();
            if (!entry->_idleSince)
            {
                entry->_idleSince = _epoch;
            }
        }
    }
    for (auto& entry : _images)
    {
        if (retired(entry.completion) && entry.image.use_count() == 1)
        {
            if (!entry.idleSince)
            {
                entry.idleSince = _epoch;
            }
        }
        else
        {
            entry.idleSince = 0;
        }
    }
    trim();
}

TemporaryPoolStatistics TemporaryResourcePool::statistics() const
{
    TemporaryPoolStatistics result;
    result.images = _images.size();
    result.bindings = _bindings.size();
    for (const auto& entry : _images)
    {
        result.estimatedBytes += entry.bytes;
    }
    for (const auto& entry : _bindings)
    {
        result.estimatedBytes += entry->_uniformBytes;
    }
    return result;
}

void TemporaryResourcePool::trim()
{
    uint64_t bytes = 0, bufferBytes = 0;
    for (const auto& entry : _images)
    {
        bytes += entry.bytes;
    }
    for (const auto& entry : _bindings)
    {
        bufferBytes += entry->_uniformBytes;
    }
    for (auto it = _bindings.begin(); it != _bindings.end();)
    {
        const auto& entry = *it;
        if (entry.use_count() == 1 && entry->_resources.use_count() == 1 && retired(entry->_completion) &&
            (!_enabled || bufferBytes + bytes > _limits.retainedBytes || _bindings.size() > _limits.bindings ||
             (entry->_idleSince && _epoch - entry->_idleSince > _limits.idleFrames)))
        {
            bufferBytes -= entry->_uniformBytes;
            it = _bindings.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (auto it = _images.begin(); it != _images.end();)
    {
        // Completed externally held outputs may leave the cache, never cleanup().
        if (retired(it->completion) && (!it->completion->wasSubmitted() || !_enabled || bufferBytes + bytes > _limits.retainedBytes || _images.size() > _limits.images ||
            (it->idleSince && _epoch - it->idleSince > _limits.idleFrames)))
        {
            bytes -= it->bytes;
            it = _images.erase(it);
        }
        else
        {
            ++it;
        }
    }
    base::CpuProfileScope::count("temporary.retainedImageBytes", bytes, "bytes");
    base::CpuProfileScope::count("temporary.retainedUniformBytes", bufferBytes, "bytes");
    base::CpuProfileScope::count("temporary.retainedImages", _images.size());
    base::CpuProfileScope::count("temporary.retainedBindings", _bindings.size());
}

std::shared_ptr<gpu::Image> TemporaryResourcePool::image(gpu::CommandBuffer& command, const gpu::ImageDescription& description)
{
    if (!command.isValid() || !command.recordingCompletion())
    {
        throw std::invalid_argument("Temporary images require a native recording ticket");
    }
    const auto bytes = imageBytes(description);
    if (_enabled)
    {
        for (auto& entry : _images)
        {
            const auto& image = entry.image;
            if (image.use_count() == 1 && entry.completion && entry.completion->completed() && image->isValid() &&
                image->bindingGeneration() == entry.generation && image->size() == description.size &&
                image->pixelFormat() == description.format && image->usage() == description.usage &&
                image->imageType() == description.type && image->mipLevels() == gpu::effectiveMipLevels(description))
            {
                entry.completion = command.recordingCompletion();
                entry.idleSince = 0;
                base::CpuProfileScope::count("temporary.imageHits", 1);
                return image;
            }
        }
    }
    auto image = _device->createImage(description);
    _images.push_back({image, command.recordingCompletion(), image->bindingGeneration(), bytes, 0});
    base::CpuProfileScope::count("temporary.imageAllocations", 1, "allocations");
    return image;
}

std::shared_ptr<TemporaryBindings> TemporaryResourcePool::bindings(gpu::CommandBuffer& command,
    std::shared_ptr<gpu::PipelineLayout> layout, uint32_t set, uint64_t uniformBytes, std::array<uint64_t,4> identity)
{
    if (!command.isValid() || !command.recordingCompletion() || !layout || !layout->isValid() || layout->device() != _device)
    {
        throw std::invalid_argument("Temporary binding layout must match the pool device");
    }
    if (_enabled)
    {
        for (auto& entry : _bindings)
        {
            if (entry.use_count() == 1 && entry->_resources.use_count() == 1 && retired(entry->_completion) && entry->_layout == layout &&
                entry->_set == set && entry->_identity == identity && entry->_uniformBytes == uniformBytes && entry->_resources->isValid())
            {
                entry->releaseOwners();
                if (entry->_resources.use_count() != 1 || (entry->_uniforms && entry->_uniforms.use_count() != 1))
                {
                    continue;
                }
                entry->_completion = command.recordingCompletion();
                entry->_idleSince = 0;
                base::CpuProfileScope::count("temporary.bindingHits", 1);
                return entry;
            }
        }
    }
    auto entry = std::make_shared<TemporaryBindings>();
    entry->_layout = std::move(layout);
    entry->_set = set;
    entry->_identity = identity;
    entry->_uniformBytes = uniformBytes;
    if (uniformBytes)
    {
        entry->_uniforms = _device->createBuffer("draw pooled invocation uniforms");
    }
    entry->_resources = _device->createResourceSet(entry->_layout.get(), set);
    entry->_dirty = true;
    entry->_completion = command.recordingCompletion();
    _bindings.push_back(entry);
    base::CpuProfileScope::count("temporary.bindingAllocations", 1, "allocations");
    return entry;
}
}
