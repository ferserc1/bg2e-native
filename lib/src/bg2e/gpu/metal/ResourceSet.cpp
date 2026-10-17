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
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/Sampler.hpp>
#include <bg2e/gpu/metal/Buffer.hpp>
#include <bg2e/gpu/metal/RayTracingScene.hpp>

#include <algorithm>
#include <map>
#include <stdexcept>

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
    auto& array = arrayBinding(binding, ResourceType::SampledImage, views.size());
    std::vector<TextureHandle> images;
    for (const auto& view : views)
    {
        auto* image = dynamic_cast<metal::Image*>(view.image);
        if (!image || image->device() != device() || !image->isValid() ||
            !hasFlag(image->usage(), ImageUsage::Sampled) || image->imageType() != ImageType::Image2D ||
            isDepthFormat(image->pixelFormat()))
        {
            throw std::invalid_argument("Argument image must be a valid same-device sampled color 2D view");
        }
        image->validateRange(view.range);
        images.push_back(image->subresourceView(view.range));
    }
    array.images = std::move(images);
    _arraysDirty = true;
}

void ResourceSet::setSamplers(ShaderBinding binding, std::span<gpu::Sampler* const> values)
{
    auto& array = arrayBinding(binding, ResourceType::Sampler, values.size());
    std::vector<SampleStateHandle> samplers;
    for (auto* value : values)
    {
        auto* sampler = dynamic_cast<metal::Sampler*>(value);
        if (!sampler || sampler->device() != device() || !sampler->isValid())
        {
            throw std::invalid_argument("Argument sampler must be valid and belong to this device");
        }
        samplers.push_back(sampler->handle());
    }
    array.samplers = std::move(samplers);
    _arraysDirty = true;
}

void ResourceSet::update()
{
    if (_arrays.empty())
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
    auto* nativeDevice = dynamic_cast<metal::Device*>(device());
    try
    {
        for (const auto& [slot, arrays] : groups)
        {
            std::vector<NS::Object*> descriptors;
            for (const auto* array : arrays)
            {
                auto* descriptor = MTL::ArgumentDescriptor::alloc()->init();
                descriptor->setIndex(array->description.binding.metal);
                descriptor->setArrayLength(array->description.count);
                descriptor->setAccess(MTL::BindingAccessReadOnly);
                descriptor->setDataType(array->description.type == ResourceType::SampledImage ? MTL::DataTypeTexture : MTL::DataTypeSampler);
                if (array->description.type == ResourceType::SampledImage)
                {
                    descriptor->setTextureType(MTL::TextureType2D);
                }
                descriptors.push_back(descriptor);
            }
            auto* arguments = NS::Array::alloc()->init(descriptors.data(), descriptors.size());
            auto* encoder = nativeDevice->handle()->newArgumentEncoder(arguments);
            arguments->release();
            for (auto* descriptor : descriptors)
            {
                descriptor->release();
            }
            if (!encoder)
            {
                throw std::runtime_error("Failed to create Metal argument encoder");
            }
            auto* buffer = nativeDevice->handle()->newBuffer(encoder->encodedLength(), MTL::ResourceStorageModeShared);
            if (!buffer)
            {
                encoder->release();
                throw std::runtime_error("Failed to allocate Metal argument buffer");
            }
            buffers.push_back(buffer);
            encoder->setArgumentBuffer(buffer, 0);
            ResourceEntry entry;
            entry.index = slot;
            entry.stage = arrays.front()->description.stage;
            entry.type = ResourceType::StorageBuffer;
            entry.buffer = buffer;
            for (const auto* array : arrays)
            {
                for (size_t i = 0; i < array->images.size(); ++i)
                {
                    encoder->setTexture(array->images[i], array->description.binding.metal + i);
                    entry.indirectTextures.push_back(array->images[i]);
                }
                for (size_t i = 0; i < array->samplers.size(); ++i)
                {
                    encoder->setSamplerState(array->samplers[i], array->description.binding.metal + i);
                }
            }
            encoder->release();
            entries.push_back(std::move(entry));
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
    _entries.erase(std::remove_if(_entries.begin(), _entries.end(), [this](const auto& entry)
        {
            return std::find(_argumentBuffers.begin(), _argumentBuffers.end(), entry.buffer) != _argumentBuffers.end();
        }), _entries.end());
    for (auto* buffer : _argumentBuffers)
    {
        buffer->release();
    }
    _argumentBuffers = std::move(buffers);
    _entries.insert(_entries.end(), entries.begin(), entries.end());
    _arraysDirty = false;
}

bool ResourceSet::isValid() const
{
    return _layout != nullptr;
}

void ResourceSet::cleanup()
{
    for (auto* buffer : _argumentBuffers)
    {
        buffer->release();
    }
    _argumentBuffers.clear();
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
