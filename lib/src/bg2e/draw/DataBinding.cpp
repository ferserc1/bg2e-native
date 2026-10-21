/*
 *    business grade graphic engine (bg2 engine)
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

#include <bg2e/draw/FrameDataBinding.hpp>
#include <bg2e/draw/ObjectDataBinding.hpp>
#include <bg2e/draw/LightDataBinding.hpp>
#include <bg2e/draw/EnvironmentDataBinding.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
namespace {
uint32_t slotCount(Engine& engine, uint32_t count)
{
    if (count == 0)
    {
        count = engine.frameSlotCount();
    }
    if (count == 0)
    {
        throw std::invalid_argument("Data bindings need at least one acquired frame slot");
    }
    return count;
}

void validateLayout(Engine& engine, const std::shared_ptr<gpu::PipelineLayout>& layout)
{
    if (!layout || !layout->isValid() || layout->device() != engine.device())
    {
        throw std::invalid_argument("Draw binding layout must belong to its engine");
    }
}

void validateTexture(const std::shared_ptr<Texture>& texture, Engine& engine)
{
    if (!texture || texture->engine() != &engine || !texture->image() || !texture->image()->isValid() ||
        texture->image()->device() != engine.device() || texture->image()->isCubemap() ||
        !texture->sampler() || !texture->sampler()->isValid() || texture->sampler()->device() != engine.device() ||
        texture->image()->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
    {
        throw std::invalid_argument("Object texture realization is stale or belongs to another context");
    }
}
}

gpu::PipelineLayoutDescription drawBindingLayout()
{
    gpu::PipelineLayoutDescription result;
    auto add = [&](uint32_t set, uint32_t vk, uint32_t metal, gpu::ResourceType type, gpu::ShaderStage stage)
    {
        result.resourceBindings.push_back({set, {.vulkan = vk, .metal = metal}, type, stage, 1});
    };
    add(0, 0, 2, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex);
    add(0, 1, 6, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    add(1, 0, 3, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex);
    add(1, 1, 7, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    for (uint32_t i = 0; i < 6; ++i)
    {
        add(1, 2 + i, i, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment);
        add(1, 8 + i, i, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment);
    }
    add(2, 0, 4, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    for (uint32_t i = 0; i < 4; ++i)
    {
        add(3, i, 6 + i, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment);
        add(3, 4 + i, 6 + i, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment);
    }
    add(3, 8, 5, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    result.debugName = "draw.bindings.v1";
    return result;
}

void DataBinding::initializeUniforms(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout,
    uint32_t setIndex, size_t byteSize, const std::vector<gpu::ShaderBinding>& bindings, uint32_t frameSlots)
{
    if (_engine && _engine != &engine)
    {
        throw std::logic_error("Clean up data bindings before changing engines");
    }
    validateLayout(engine, layout);
    const auto count = slotCount(engine, frameSlots);
    cleanup();
    _engine = &engine;
    _layout = std::move(layout);
    _byteSize = byteSize;
    _slots.resize(count);
    std::vector<uint8_t> zero(byteSize, 0);
    for (auto& slot : _slots)
    {
        slot.buffer = engine.device()->createBuffer("Draw frame uniform data");
        slot.buffer->createUniformBuffer(zero.data(), byteSize);
        slot.set = engine.device()->createResourceSet(_layout.get(), setIndex);
        for (const auto binding : bindings)
        {
            slot.set->setUniformBuffer(binding, slot.buffer.get());
        }
        slot.set->update();
    }
}

DataBinding::Slot& DataBinding::updateBytes(const FrameContext& frame, const void* data, size_t byteSize)
{
    if (_engine != &frame.engine || frame.frameSlot >= _slots.size() || byteSize != _byteSize || !data)
    {
        throw std::invalid_argument("Data binding update does not match its acquired frame slot");
    }
    auto& slot = _slots.at(frame.frameSlot);
    if (slot.prepared && slot.frameNumber == frame.frameNumber)
    {
        throw std::logic_error("Do not overwrite mutable data already prepared for this frame");
    }
    if (!slot.buffer || !slot.set)
    {
        throw std::logic_error("Data binding initialization did not complete");
    }
    slot.buffer->updateUniformBuffer(data, byteSize);
    slot.frameNumber = frame.frameNumber;
    slot.prepared = true;
    return slot;
}

gpu::ResourceSet* DataBinding::set(const FrameContext& frame) const
{
    if (&frame.engine != _engine || frame.frameSlot >= _slots.size())
    {
        throw std::invalid_argument("Data binding belongs to another context/frame slot");
    }
    const auto& slot = _slots.at(frame.frameSlot);
    if (!slot.prepared || slot.frameNumber != frame.frameNumber || !slot.set)
    {
        throw std::logic_error("Prepare this slot before binding its frame data");
    }
    return slot.set.get();
}

void DataBinding::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _layout.reset();
    _byteSize = 0;
    _engine = nullptr;
}

void FrameDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots)
{
    initializeUniforms(engine, std::move(layout), 0, sizeof(FrameData), {{.vulkan = 0, .metal = 2}, {.vulkan = 1, .metal = 6}}, frameSlots);
}

void FrameDataBinding::update(const FrameContext& frame, const glm::mat4& view, const glm::mat4& projection)
{
    const auto data = packFrame(frame, view, projection);
    updateBytes(frame, &data, sizeof(data));
}

void LightDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots)
{
    initializeUniforms(engine, std::move(layout), 2, sizeof(LightUniforms), {{.vulkan = 0, .metal = 4}}, frameSlots);
}

void LightDataBinding::update(const FrameContext& frame, const LightUniforms& lights)
{
    if (lights.count.x > MaxLights)
    {
        throw std::length_error("Draw light binding supports at most eight lights");
    }
    for (uint32_t n = 0; n < lights.count.x; ++n)
    {
        for (const auto& value : {lights.lights[n].positionIntensity, lights.lights[n].color, lights.lights[n].directionAngle, lights.lights[n].parameters})
        {
            for (int i = 0; i < 4; ++i)
            {
                if (!std::isfinite(value[i]))
                {
                    throw std::invalid_argument("Light data must be finite");
                }
            }
        }
    }
    updateBytes(frame, &lights, sizeof(lights));
}

void EnvironmentDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots, uint32_t metalTextureBase)
{
    initializeUniforms(engine, std::move(layout), 3, sizeof(EnvironmentData), {{.vulkan = 8, .metal = 5}}, frameSlots);
    _metalTextureBase = metalTextureBase;
    _records.resize(_slots.size());
}

void EnvironmentDataBinding::update(const FrameContext& frame, std::shared_ptr<const environment::EnvironmentResources> environment)
{
    if (!environment || _engine != &frame.engine || frame.frameSlot >= _records.size())
    {
        throw std::invalid_argument("Environment frame binding requires a compatible resource snapshot");
    }
    auto& slot = _slots.at(frame.frameSlot);
    if (slot.prepared && slot.frameNumber == frame.frameNumber)
    {
        throw std::logic_error("Do not overwrite environment data already prepared for this frame");
    }
    environment->validate(frame.engine.device());
    EnvironmentData data;
    data.parameters = {float(environment->prefilteredSpecular->mipLevels() - 1), environment->skyboxBlur, environment->skyboxVisible ? 1.0f : 0.0f, 0};
    const std::array<std::shared_ptr<gpu::Image>, 4> images = {environment->sky, environment->irradiance, environment->prefilteredSpecular, environment->brdf};
    const std::array<std::shared_ptr<gpu::Sampler>, 4> samplers = {environment->sampler, environment->sampler, environment->sampler, environment->brdfSampler};
    std::array<uint64_t, 4> generations{};
    auto& record = _records.at(frame.frameSlot);
    bool bindingsChanged = !_reuseEnabled || !record.valid;
    for (size_t i = 0; i < images.size(); ++i)
    {
        generations[i] = images[i]->bindingGeneration();
        bindingsChanged |= record.images[i] != images[i] || record.samplers[i] != samplers[i] || record.imageGenerations[i] != generations[i];
    }
    const bool uniformChanged = !_reuseEnabled || !record.valid || std::memcmp(&record.data, &data, sizeof(data)) != 0;
    const bool wasValid = _reuseEnabled && record.valid;
    slot.prepared = false; // Do not expose a candidate until every update succeeds.
    record.valid = false;
    try
    {
        if (!slot.set)
        {
            slot.set = frame.engine.device()->createResourceSet(_layout.get(), 3);
            slot.set->setUniformBuffer({.vulkan = 8, .metal = 5}, slot.buffer.get());
        }
        if (uniformChanged)
        {
            slot.buffer->updateUniformBuffer(data);
            base::CpuProfileScope::count("environmentBinding.uniformUploads", 1);
        }
        if (bindingsChanged)
        {
            for (uint32_t i = 0; i < images.size(); ++i)
            {
                if (!wasValid || record.images[i] != images[i] || record.imageGenerations[i] != generations[i])
                {
                    slot.set->setSampledImage({.vulkan = i, .metal = _metalTextureBase + i}, images[i].get());
                }
                if (!wasValid || record.samplers[i] != samplers[i])
                {
                    slot.set->setSampler({.vulkan = 4 + i, .metal = _metalTextureBase + i}, samplers[i].get());
                }
            }
            slot.set->update();
            base::CpuProfileScope::count("environmentBinding.descriptorUpdates", 1);
        }
        else
        {
            base::CpuProfileScope::count("environmentBinding.bindingCacheHits", 1);
        }
        record.owner = std::move(environment);
        record.images = images;
        record.samplers = samplers;
        record.imageGenerations = generations;
        record.data = data;
        record.valid = true;
    }
    catch (...)
    {
        // Discard native pending writes as well as the unpublished cache state.
        slot.set.reset();
        record.valid = false;
        throw;
    }
    slot.frameNumber = frame.frameNumber;
    slot.prepared = true;
}

void EnvironmentDataBinding::cleanup()
{
    DataBinding::cleanup();
    _records.clear();
}

void ObjectDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots)
{
    if (_engine && _engine != &engine)
    {
        throw std::logic_error("Clean up object bindings before changing engines");
    }
    validateLayout(engine, layout);
    const auto count = slotCount(engine, frameSlots);
    cleanup();
    _engine = &engine;
    _layout = std::move(layout);
    _slots.resize(count);
}

void ObjectDataBinding::prepare(const FrameContext& frame, const RenderQueue& queue)
{
    if (_engine != &frame.engine || queue.engine() != _engine || frame.frameSlot >= _slots.size())
    {
        throw std::invalid_argument("Object queue and acquired frame slot must share an engine");
    }
    queue.validate(frame.engine);
    auto& slot = _slots.at(frame.frameSlot);
    if (slot.prepared && slot.frameNumber == frame.frameNumber)
    {
        throw std::logic_error("Do not rewrite object data already prepared for this frame");
    }
    for (const auto& item : queue.items())
    {
        for (const auto& texture : item.textures)
        {
            validateTexture(texture, frame.engine);
        }
    }
    // Preflight the complete queue before touching any record in the acquired slot.
    for (size_t i = 0; i < queue.items().size(); ++i)
    {
        if (queue.items()[i].objectId != i + 1)
        {
            throw std::invalid_argument("Object IDs must index the complete queue snapshot");
        }
    }
    slot.prepared = false;
    slot.records.resize(queue.items().size());
    for (size_t i = 0; i < queue.items().size(); ++i)
    {
        const auto& item = queue.items()[i];
        auto& record = slot.records[i];
        const bool sameSource = _reuseEnabled && record.valid && record.owner.nodeId == item.nodeId &&
            record.owner.submeshIndex == item.submeshIndex && record.owner.drawMesh == item.drawMesh &&
            record.owner.drawMaterial == item.drawMaterial;
        const bool wasValid = record.valid;
        const bool newBuffer = !record.buffer || !record.buffer->isValid();
        const bool newSet = !record.set;
        bool bindingsChanged = !sameSource || newBuffer || newSet || record.owner.materialRevisions.bindings != item.materialRevisions.bindings;
        ObjectData data;
        data.model = item.worldMatrix;
        data.normal = sameSource && record.data.model == item.worldMatrix ? record.data.normal : glm::transpose(glm::inverse(item.worldMatrix));
        data.material = item.materialData;
        data.identity = {item.objectId, item.submeshIndex, 0, 0};
        for (size_t texture = 0; texture < item.textures.size(); ++texture)
        {
            bindingsChanged |= record.images[texture].get() != item.textureImages[texture] ||
                record.samplers[texture].get() != item.textureSamplers[texture] ||
                record.owner.textureRevisions[texture] != item.textureRevisions[texture] ||
                record.owner.imageGenerations[texture] != item.imageGenerations[texture];
        }
        record.valid = false;
        try
        {
            if (newBuffer)
            {
                auto buffer = frame.engine.device()->createBuffer("Draw object uniform data");
                buffer->createUniformBuffer(data);
                record.buffer = std::move(buffer);
                base::CpuProfileScope::count("objectBinding.uniformUploads", 1);
            }
            else if (!_reuseEnabled || !wasValid || record.owner.materialRevisions.values != item.materialRevisions.values ||
                std::memcmp(&record.data, &data, sizeof(data)) != 0)
            {
                record.buffer->updateUniformBuffer(data);
                base::CpuProfileScope::count("objectBinding.uniformUploads", 1);
            }
            if (newSet)
            {
                record.set = frame.engine.device()->createResourceSet(_layout.get(), 1);
            }
            if (!wasValid || newBuffer || newSet)
            {
                record.set->setUniformBuffer({.vulkan = 0, .metal = 3}, record.buffer.get());
                record.set->setUniformBuffer({.vulkan = 1, .metal = 7}, record.buffer.get());
            }
            if (bindingsChanged)
            {
                for (uint32_t texture = 0; texture < item.textures.size(); ++texture)
                {
                    if (newSet || !sameSource || record.images[texture].get() != item.textureImages[texture] ||
                        record.owner.imageGenerations[texture] != item.imageGenerations[texture] ||
                        record.owner.textureRevisions[texture] != item.textureRevisions[texture])
                    {
                        record.set->setSampledImage({.vulkan = 2 + texture, .metal = texture}, item.textureImages[texture]);
                    }
                    if (newSet || !sameSource || record.samplers[texture].get() != item.textureSamplers[texture] ||
                        record.owner.textureRevisions[texture] != item.textureRevisions[texture])
                    {
                        record.set->setSampler({.vulkan = 8 + texture, .metal = texture}, item.textureSamplers[texture]);
                    }
                }
                record.set->update();
                base::CpuProfileScope::count("objectBinding.descriptorUpdates", 1);
            }
            else
            {
                base::CpuProfileScope::count("objectBinding.bindingCacheHits", 1);
            }
            record.owner = item;
            record.data = data;
            for (size_t texture = 0; texture < item.textures.size(); ++texture)
            {
                record.images[texture] = item.textures[texture]->imageResource();
                record.samplers[texture] = item.textures[texture]->samplerResource();
            }
            record.valid = true;
        }
        catch (...)
        {
            record.set.reset();
            record.valid = false;
            throw;
        }
    }
    slot.frameNumber = frame.frameNumber;
    slot.prepared = true;
}

gpu::ResourceSet* ObjectDataBinding::set(const FrameContext& frame, uint32_t objectId) const
{
    if (_engine != &frame.engine || frame.frameSlot >= _slots.size())
    {
        throw std::invalid_argument("Object data belongs to another acquired frame slot");
    }
    const auto& slot = _slots.at(frame.frameSlot);
    if (!slot.prepared || slot.frameNumber != frame.frameNumber || objectId == 0 || objectId > slot.records.size())
    {
        throw std::out_of_range("Prepare the object snapshot before binding its one-based ID");
    }
    return slot.records.at(objectId - 1).set.get();
}

void ObjectDataBinding::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _layout.reset();
    _engine = nullptr;
}
}
