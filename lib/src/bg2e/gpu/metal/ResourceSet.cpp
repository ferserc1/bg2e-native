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

#include <bg2e/gpu/metal/ResourceSet.hpp>
#include "ArgumentEncoderState.hpp"
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/Sampler.hpp>
#include <bg2e/gpu/metal/Buffer.hpp>
#include <bg2e/gpu/metal/RayTracingScene.hpp>

#include <algorithm>
#include <map>
#include <stdexcept>
#include <unordered_set>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

ResourceSet::ResourceSet(gpu::Device* gpuDevice, const metal::PipelineLayout* layout, uint32_t setIndex)
    : gpu::ResourceSet(gpuDevice), _setIndex(setIndex), _layout(layout)
{
    for (const auto& rb : layout->resourceBindings())
    {
        if (rb.set == setIndex && rb.metalArgumentBuffer != UINT32_MAX)
        {
            ArrayBinding array;
            array.description = rb;
            _arrays.push_back(array);
        }
    }
}

ResourceSet::~ResourceSet()
{
    cleanup();
}

void ResourceSet::setStorageImage(ShaderBinding binding, gpu::Image* image)
{
    if (!image)
    {
        throw std::invalid_argument("Image binding requires an image");
    }
    setStorageImage(binding, image->view({0, 1, 0, image->arrayLayers()}));
}

void ResourceSet::setStorageImage(ShaderBinding binding, const ImageView& view)
{
    auto* image = view.image;
    if (!image || image->device() != device() || !hasFlag(image->usage(), ImageUsage::Storage))
    {
        throw std::invalid_argument("Image view device/usage is incompatible with this resource set");
    }
    image->view(view.range);
    if (view.range.mipCount != 1 || isDepthFormat(image->pixelFormat()))
    {
        throw std::invalid_argument("A storage image view selects exactly one color mip");
    }
    auto* metalImage = dynamic_cast<metal::Image*>(image);
    if (!metalImage)
    {
        throw std::runtime_error("metal::ResourceSet::setStorageImage: invalid image");
    }

    // Find matching binding in layout
    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.metalArgumentBuffer == UINT32_MAX && rb.count == 1 && rb.binding.metal == binding.metal && rb.type == ResourceType::StorageImage)
        {
            ResourceEntry entry;
            entry.index = _layout->metalIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.texture = metalImage->subresourceView(view.range);

            // Replace existing entry or append
            for (auto& e : _entries)
            {
                if (e.index == entry.index && e.type == ResourceType::StorageImage)
                {
                    e = entry;
                    return;
                }
            }
            _entries.push_back(entry);
            return;
        }
    }

    throw std::runtime_error("metal::ResourceSet::setStorageImage: no matching binding in layout");
}

void ResourceSet::setSampledImage(ShaderBinding binding, gpu::Image* image)
{
    if (!image)
    {
        throw std::invalid_argument("Image binding requires an image");
    }
    setSampledImage(binding, image->fullView());
}

void ResourceSet::setSampledImage(ShaderBinding binding, const ImageView& view)
{
    auto* image = view.image;
    if (!image || image->device() != device() || !hasFlag(image->usage(), ImageUsage::Sampled))
    {
        throw std::invalid_argument("Image view device/usage is incompatible with this resource set");
    }
    image->view(view.range);
    auto* metalImage = dynamic_cast<metal::Image*>(image);
    if (!metalImage)
    {
        throw std::runtime_error("metal::ResourceSet::setSampledImage: invalid image");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.metalArgumentBuffer == UINT32_MAX && rb.count == 1 && rb.binding.metal == binding.metal && rb.type == ResourceType::SampledImage)
        {
            ResourceEntry entry;
            entry.index = _layout->metalIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.texture = metalImage->subresourceView(view.range);

            for (auto& e : _entries)
            {
                if (e.index == entry.index && e.type == ResourceType::SampledImage)
                {
                    e = entry;
                    return;
                }
            }
            _entries.push_back(entry);
            return;
        }
    }

    throw std::runtime_error("metal::ResourceSet::setSampledImage: no matching binding in layout");
}

void ResourceSet::setSampler(ShaderBinding binding, gpu::Sampler* sampler)
{
    auto* metalSampler = dynamic_cast<metal::Sampler*>(sampler);
    if (!metalSampler || metalSampler->device() != device() || !metalSampler->isValid())
    {
        throw std::runtime_error("metal::ResourceSet::setSampler: invalid sampler");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.metalArgumentBuffer == UINT32_MAX && rb.count == 1 && rb.binding.metal == binding.metal && rb.type == ResourceType::Sampler)
        {
            ResourceEntry entry;
            entry.index = _layout->metalIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.sampler = metalSampler->handle();

            for (auto& e : _entries)
            {
                if (e.index == entry.index && e.type == ResourceType::Sampler)
                {
                    e = entry;
                    return;
                }
            }
            _entries.push_back(entry);
            return;
        }
    }

    throw std::runtime_error("metal::ResourceSet::setSampler: no matching binding in layout");
}

void ResourceSet::setUniformBuffer(ShaderBinding binding, gpu::Buffer* buffer)
{
    setBufferBinding(binding, buffer, ResourceType::UniformBuffer, "setUniformBuffer");
}

void ResourceSet::setStorageBuffer(ShaderBinding binding, gpu::Buffer* buffer)
{
    setBufferBinding(binding, buffer, ResourceType::StorageBuffer, "setStorageBuffer");
}

void ResourceSet::setRayTracingScene(ShaderBinding binding, gpu::RayTracingScene* scene)
{
    auto* metalScene = dynamic_cast<metal::RayTracingScene*>(scene);
    if (!metalScene || metalScene->device() != device() || !metalScene->isValid())
    {
        throw std::runtime_error("metal::ResourceSet::setRayTracingScene: invalid ray tracing scene");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.metalArgumentBuffer == UINT32_MAX && rb.count == 1 && rb.binding.metal == binding.metal &&
            rb.type == ResourceType::AccelerationStructure)
        {
            ResourceEntry entry;
            entry.index = _layout->metalBufferIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.rtScene = metalScene;

            for (auto& e : _entries)
            {
                if (e.index == entry.index && e.type == ResourceType::AccelerationStructure)
                {
                    e = entry;
                    return;
                }
            }
            _entries.push_back(entry);
            return;
        }
    }

    throw std::runtime_error("metal::ResourceSet::setRayTracingScene: no matching binding in layout");
}

void ResourceSet::setBufferBinding(ShaderBinding binding, gpu::Buffer* buffer,
                                   ResourceType type, const char* debugTag)
{
    auto* metalBuffer = dynamic_cast<metal::Buffer*>(buffer);
    if (!metalBuffer || metalBuffer->device() != device() || !metalBuffer->isValid() || !buffer->byteSize() ||
        !hasFlag(buffer->usage(), type == ResourceType::StorageBuffer ? BufferUsage::Storage : BufferUsage::Uniform) ||
        (type == ResourceType::StorageBuffer && buffer->byteSize() > device()->resourceArrayCapabilities().maxStorageBufferRange))
    {
        throw std::runtime_error(std::string("metal::ResourceSet::") + debugTag + ": invalid buffer");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.metalArgumentBuffer == UINT32_MAX && rb.count == 1 && rb.binding.metal == binding.metal && rb.type == type)
        {
            ResourceEntry entry;
            entry.index = _layout->metalBufferIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.buffer = metalBuffer->handle();

            for (auto& e : _entries)
            {
                if (e.index == entry.index && e.type == type)
                {
                    e = entry;
                    return;
                }
            }
            _entries.push_back(entry);
            return;
        }
    }

    throw std::runtime_error(std::string("metal::ResourceSet::") + debugTag + ": no matching binding in layout");
}

ResourceSet::ArrayBinding& ResourceSet::arrayBinding(ShaderBinding binding, ResourceType type, size_t count)
{
    for (auto& array : _arrays)
    {
        const auto& rb = array.description;
        if (rb.binding.vulkan == binding.vulkan && rb.binding.metal == binding.metal && rb.type == type && rb.count == count)
        {
            return array;
        }
    }
    throw std::invalid_argument("Argument-array type/count/binding does not match the layout");
}

void ResourceSet::setSampledImages(ShaderBinding binding, std::span<const ImageView> views)
{
    base::CpuProfileScope profile("metal.resourceSet.resolveImageViews");
    auto& array = arrayBinding(binding, ResourceType::SampledImage, views.size());
    std::vector<TextureHandle> images;
    std::vector<ImageKey> keys;
    std::unordered_map<ImageKey, ResolvedImage, ImageHash> resolved;
    images.reserve(views.size());
    keys.reserve(views.size());
    uint64_t resolutions = 0;
    for (const auto& view : views)
    {
        if (!view.image)
        {
            throw std::invalid_argument("Argument image view requires an image");
        }
        const ImageKey key{view.image, view.image->bindingGeneration(), view.range};
        auto found = resolved.find(key);
        if (found == resolved.end())
        {
            auto* image = dynamic_cast<metal::Image*>(view.image);
            if (!image || image->device() != device() || !image->isValid() ||
                !hasFlag(image->usage(), ImageUsage::Sampled) || image->imageType() != ImageType::Image2D ||
                isDepthFormat(image->pixelFormat()))
            {
                throw std::invalid_argument("Argument image must be a valid same-device sampled color 2D view");
            }
            image->validateRange(view.range);
            ResolvedImage entry;
            const auto previous = array.imageCache.find(key);
            if (previous != array.imageCache.end() && previous->second.owner)
            {
                entry = previous->second;
            }
            else
            {
                entry = {image->subresourceView(view.range), image->weak_from_this().lock()};
                ++resolutions;
            }
            found = resolved.emplace(key, std::move(entry)).first;
        }
        images.push_back(found->second.handle);
        keys.push_back(key);
    }
    const bool changed = array.imageKeys != keys || array.images != images;
    array.images = std::move(images);
    array.imageKeys = std::move(keys);
    array.imageCache = std::move(resolved);
    _arraysDirty |= changed;
    base::CpuProfileScope::count("metal.argumentArray.resolvedImageEntries", resolutions);
}

void ResourceSet::setSamplers(ShaderBinding binding, std::span<gpu::Sampler* const> values)
{
    base::CpuProfileScope profile("metal.resourceSet.resolveSamplers");
    auto& array = arrayBinding(binding, ResourceType::Sampler, values.size());
    std::vector<SampleStateHandle> samplers;
    std::vector<gpu::Sampler*> keys;
    std::unordered_map<gpu::Sampler*, ResolvedSampler> resolved;
    samplers.reserve(values.size());
    keys.reserve(values.size());
    uint64_t resolutions = 0;
    for (auto* value : values)
    {
        auto found = resolved.find(value);
        if (found == resolved.end())
        {
            auto* sampler = dynamic_cast<metal::Sampler*>(value);
            if (!sampler || sampler->device() != device() || !sampler->isValid())
            {
                throw std::invalid_argument("Argument sampler must be valid and belong to this device");
            }
            ResolvedSampler entry;
            const auto previous = array.samplerCache.find(value);
            if (previous != array.samplerCache.end() && previous->second.owner)
            {
                entry = previous->second;
            }
            else
            {
                entry = {sampler->handle(), sampler->weak_from_this().lock()};
                ++resolutions;
            }
            found = resolved.emplace(value, std::move(entry)).first;
        }
        samplers.push_back(found->second.handle);
        keys.push_back(value);
    }
    const bool changed = array.samplerKeys != keys || array.samplers != samplers;
    array.samplers = std::move(samplers);
    array.samplerKeys = std::move(keys);
    array.samplerCache = std::move(resolved);
    _arraysDirty |= changed;
    base::CpuProfileScope::count("metal.argumentArray.resolvedSamplerEntries", resolutions);
}

void ResourceSet::update()
{
    base::CpuProfileScope profile("metal.resourceSet.updateArgumentArrays");
    if (_arrays.empty() || (!_arraysDirty && !_argumentBuffers.empty()))
    {
        return;
    }
    std::map<uint32_t, std::vector<const ArrayBinding*>> groups;
    for (const auto& array : _arrays)
    {
        if (array.images.size() + array.samplers.size() != array.description.count)
        {
            throw std::invalid_argument("Every argument-array element must be initialized before update");
        }
        groups[array.description.metalArgumentBuffer].push_back(&array);
    }
    if (!_arraysDirty)
    {
        return;
    }
    std::vector<BufferHandle> buffers;
    std::vector<ResourceEntry> entries;
    std::vector<ResourceEntry> replacement;
    std::vector<std::shared_ptr<gpu::DeviceResource>> owners;
    buffers.reserve(groups.size());
    entries.reserve(groups.size());
    auto* nativeDevice = dynamic_cast<metal::Device*>(device());
    try
    {
        for (const auto& [slot, arrays] : groups)
        {
            const bool records = arrays.front()->description.metalArgumentRecordsIndexable;
            const auto recordCount = records ? arrays.front()->description.count : 1u;
            base::CpuProfileScope arrayProfile("metal.argumentArray.getEncoder");
            const auto encoderState = _layout->argumentEncoder(slot);
            // A layout may serve concurrent sets/jobs. Never let their mutable
            // argument-buffer targets overlap inside the shared native encoder.
            std::lock_guard encoderLock(encoderState->mutex);
            auto* encoder = encoderState->encoder;
            const auto stride = encoder->encodedLength();
            if (!stride || recordCount > nativeDevice->handle()->maxBufferLength() / stride)
            {
                throw std::length_error("Metal argument table exceeds maxBufferLength");
            }
            arrayProfile.next("metal.argumentArray.allocateBuffer");
            auto* buffer = nativeDevice->handle()->newBuffer(stride * recordCount, MTL::ResourceStorageModeShared);
            if (!buffer)
            {
                throw std::runtime_error("Failed to allocate Metal argument buffer");
            }
            buffers.push_back(buffer);
            encoder->setArgumentBuffer(buffer, 0);
            ResourceEntry entry;
            entry.index = slot;
            entry.stage = arrays.front()->description.stage;
            entry.type = ResourceType::StorageBuffer;
            entry.buffer = buffer;
            arrayProfile.next("metal.argumentArray.encodeEntries");
            std::unordered_set<TextureHandle> residentTextures;
            for (const auto* array : arrays)
            {
                for (size_t i = 0; i < array->images.size(); ++i)
                {
                    if (records)
                    {
                        encoder->setArgumentBuffer(buffer, stride * i);
                    }
                    encoder->setTexture(array->images[i], records ? 0 : array->description.binding.metal + i);
                    if (residentTextures.insert(array->images[i]).second)
                    {
                        entry.indirectTextures.push_back(array->images[i]);
                    }
                }
                for (size_t i = 0; i < array->samplers.size(); ++i)
                {
                    if (records)
                    {
                        encoder->setArgumentBuffer(buffer, stride * i);
                    }
                    encoder->setSamplerState(array->samplers[i], records ? 0 : array->description.binding.metal + i);
                }
            }
            base::CpuProfileScope::count("metal.argumentArray.encodedEntries", [&]()
            {
                uint64_t count = 0;
                for (const auto* array : arrays)
                {
                    count += array->images.size() + array->samplers.size();
                }
                return count;
            }());
            entries.push_back(std::move(entry));
        }
        replacement = _entries;
        replacement.erase(std::remove_if(replacement.begin(), replacement.end(), [this](const auto& entry)
        {
            return std::find(_argumentBuffers.begin(), _argumentBuffers.end(), entry.buffer) != _argumentBuffers.end();
        }), replacement.end());
        replacement.insert(replacement.end(), entries.begin(), entries.end());
        std::unordered_set<gpu::DeviceResource*> retained;
        for (const auto& array : _arrays)
        {
            for (const auto& [key, image] : array.imageCache)
            {
                if (image.owner && retained.insert(image.owner.get()).second)
                {
                    owners.push_back(image.owner);
                }
            }
            for (const auto& [key, sampler] : array.samplerCache)
            {
                if (sampler.owner && retained.insert(sampler.owner.get()).second)
                {
                    owners.push_back(sampler.owner);
                }
            }
        }
    }
    catch (...)
    {
        for (auto* buffer : buffers)
        {
            buffer->release();
        }
        throw;
    }
    for (auto* buffer : _argumentBuffers)
    {
        buffer->release();
    }
    _argumentBuffers = std::move(buffers);
    _entries.swap(replacement);
    _argumentOwners = std::move(owners);
    _arraysDirty = false;
}

bool ResourceSet::isValid() const
{
    return _layout != nullptr;
}

void ResourceSet::cleanup()
{
    base::CpuProfileScope profile("metal.resourceSet.cleanup");
    for (auto* buffer : _argumentBuffers)
    {
        buffer->release();
    }
    _argumentBuffers.clear();
    _argumentOwners.clear();
    _arrays.clear();
    _arraysDirty = false;
    _entries.clear();
    _layout = nullptr;
}

#else

ResourceSet::ResourceSet(gpu::Device* gpuDevice, const metal::PipelineLayout*, uint32_t)
    : gpu::ResourceSet(gpuDevice)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

ResourceSet::~ResourceSet()
{
}

void ResourceSet::setStorageImage(ShaderBinding, const ImageView&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void ResourceSet::setSampledImage(ShaderBinding, const ImageView&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void ResourceSet::setStorageImage(ShaderBinding, gpu::Image*)
{
}

void ResourceSet::setSampledImage(ShaderBinding, gpu::Image*)
{
}

void ResourceSet::setSampler(ShaderBinding, gpu::Sampler*)
{
}

void ResourceSet::setUniformBuffer(ShaderBinding, gpu::Buffer*)
{
}

void ResourceSet::setStorageBuffer(ShaderBinding, gpu::Buffer*)
{
}

void ResourceSet::setRayTracingScene(ShaderBinding, gpu::RayTracingScene*)
{
}

void ResourceSet::setBufferBinding(ShaderBinding, gpu::Buffer*, ResourceType, const char*)
{
}

void ResourceSet::setSampledImages(ShaderBinding, std::span<const ImageView>)
{
    throw std::runtime_error("Metal backend is unavailable");
}

void ResourceSet::setSamplers(ShaderBinding, std::span<gpu::Sampler* const>)
{
    throw std::runtime_error("Metal backend is unavailable");
}

ResourceSet::ArrayBinding& ResourceSet::arrayBinding(ShaderBinding, ResourceType, size_t)
{
    throw std::runtime_error("Metal backend is unavailable");
}

void ResourceSet::update()
{
}

bool ResourceSet::isValid() const
{
    return false;
}

void ResourceSet::cleanup()
{
}

#endif

}
}
}
