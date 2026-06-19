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
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/Sampler.hpp>
#include <bg2e/gpu/metal/Buffer.hpp>
#include <bg2e/gpu/metal/RayTracingScene.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

ResourceSet::ResourceSet(gpu::Device* gpuDevice, const metal::PipelineLayout* layout, uint32_t setIndex)
    : gpu::ResourceSet(gpuDevice)
    , _setIndex(setIndex)
    , _layout(layout)
{
}

ResourceSet::~ResourceSet()
{
    cleanup();
}

void ResourceSet::setStorageImage(ShaderBinding binding, gpu::Image* image)
{
    auto* metalImage = dynamic_cast<metal::Image*>(image);
    if (!metalImage)
    {
        throw std::runtime_error("metal::ResourceSet::setStorageImage: invalid image");
    }

    // Find matching binding in layout
    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.binding.metal == binding.metal && rb.type == ResourceType::StorageImage)
        {
            ResourceEntry entry;
            entry.index = _layout->metalIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.texture = metalImage->texture();

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
    auto* metalImage = dynamic_cast<metal::Image*>(image);
    if (!metalImage)
    {
        throw std::runtime_error("metal::ResourceSet::setSampledImage: invalid image");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.binding.metal == binding.metal && rb.type == ResourceType::SampledImage)
        {
            ResourceEntry entry;
            entry.index = _layout->metalIndex(rb);
            entry.type = rb.type;
            entry.stage = rb.stage;
            entry.texture = metalImage->texture();

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
    if (!metalSampler)
    {
        throw std::runtime_error("metal::ResourceSet::setSampler: invalid sampler");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.binding.metal == binding.metal && rb.type == ResourceType::Sampler)
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
    if (!metalScene)
    {
        throw std::runtime_error("metal::ResourceSet::setRayTracingScene: invalid ray tracing scene");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.binding.metal == binding.metal &&
            rb.type == ResourceType::AccelerationStructure)
        {
            ResourceEntry entry;
            entry.index   = _layout->metalBufferIndex(rb);
            entry.type    = rb.type;
            entry.stage   = rb.stage;
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
    if (!metalBuffer)
    {
        throw std::runtime_error(std::string("metal::ResourceSet::") + debugTag + ": invalid buffer");
    }

    const auto& bindings = _layout->resourceBindings();
    for (const auto& rb : bindings)
    {
        if (rb.set == _setIndex && rb.binding.metal == binding.metal && rb.type == type)
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

void ResourceSet::update()
{
    // Metal binding happens at encode time in bindResourceSet; nothing to flush here.
}

bool ResourceSet::isValid() const
{
    return _layout != nullptr;
}

void ResourceSet::cleanup()
{
    _entries.clear();
}

#else

ResourceSet::ResourceSet(gpu::Device* gpuDevice, const metal::PipelineLayout*, uint32_t)
    : gpu::ResourceSet(gpuDevice)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

ResourceSet::~ResourceSet() {}

void ResourceSet::setStorageImage(ShaderBinding, gpu::Image*) {}
void ResourceSet::setSampledImage(ShaderBinding, gpu::Image*) {}
void ResourceSet::setSampler(ShaderBinding, gpu::Sampler*) {}
void ResourceSet::setUniformBuffer(ShaderBinding, gpu::Buffer*) {}
void ResourceSet::setStorageBuffer(ShaderBinding, gpu::Buffer*) {}
void ResourceSet::setRayTracingScene(ShaderBinding, gpu::RayTracingScene*) {}
void ResourceSet::setBufferBinding(ShaderBinding, gpu::Buffer*, ResourceType, const char*) {}
void ResourceSet::update() {}
bool ResourceSet::isValid() const { return false; }
void ResourceSet::cleanup() {}

#endif

}
}
}
